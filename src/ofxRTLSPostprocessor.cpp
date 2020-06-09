#include "ofxRTLSPostprocessor.h"

#ifdef RTLS_POSTPROCESS

// --------------------------------------------------------------
ofxRTLSPostprocessor::ofxRTLSPostprocessor() {

}

// --------------------------------------------------------------
ofxRTLSPostprocessor::~ofxRTLSPostprocessor() {

	// Flag that we should stop waiting 
	flagUnlock = true;
	// Signal the conditional variable
	cv.notify_one();

	// Stop this thread and wait for it to complete
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::setup(string _name, string _abbr, string _dictPath, 
	string _filterList) {

	name = _name;
	abbr = _abbr;
	dictPath = _dictPath;

	// Setup postprocessing params
	string ruiGroupFull = "ofxRTLSPostprocessor - " + name;
	string ruiGroupAbbr = "RTLS-P-" + abbr;
	RUI_NEW_GROUP(ruiGroupFull);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Map IDs", bMapIDs);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Remove Invalid IDs", bRemoveInvalidIDs);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Apply Hungarian", bApplyHungarian);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Apply Filters", bApplyFilters);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- ID Dict Path", dictPath);
	
	// Load the dictionary
	if (!dictPath.empty()) dict.setup(dictPath);

	// Setup the hungarian algorithm
	RUI_NEW_GROUP("Hungarian - " + abbr);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Temporary ID Fields", tempKeyTypesStr);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Permanent ID Fields", permKeyTypesStr);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Item Radius", hungarianRadius, 0, 1000000);
	vector<string> hungarianMappings = { "Temporary", "Permanent", "Both" };
	RUI_SHARE_ENUM_PARAM_WCN("HU_RTLS" + abbr + "- Mapping From", hungarianMappingFrom, HungarianMapping::TEMPORARY, HungarianMapping::TEMPORARY_AND_PERMANENT, hungarianMappings);
	RUI_SHARE_ENUM_PARAM_WCN("HU_RTLS" + abbr + "- Mapping To", hungarianMappingTo, HungarianMapping::TEMPORARY, HungarianMapping::TEMPORARY_AND_PERMANENT, hungarianMappings);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Map Recursion Limit", keyMappingRecursionLimit, 0, 10000);

	// Parse the identifiable fields and create a mapping from the string
	// type to a more mappable integer index.
	{
		vector<string> tmp = ofSplitString(tempKeyTypesStr, ",");
		for (auto& s : tmp) {
			for (int i = 0; i < NUM_KEYS - 1; i++) {
				if (s.compare(getTrackableKeyTypeDescription(TrackableKeyType(i))) == 0) {
					// match
					tempKeyTypes.insert(TrackableKeyType(i));
				}
			}
		}
	}
	{
		vector<string> tmp = ofSplitString(permKeyTypesStr, ",");
		for (auto& s : tmp) {
			for (int i = 0; i < NUM_KEYS - 1; i++) {
				if (s.compare(getTrackableKeyTypeDescription(TrackableKeyType(i))) == 0) {
					// match
					permKeyTypes.insert(TrackableKeyType(i));
				}
			}
		}
	}

	// Setup the filters
	filters.setup("RTLS-"+abbr, _filterList);

	startThread();
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::threadedFunction() {

	while (isThreadRunning()) {

		DataElem* elem = NULL;

		{
			// Lock the mutex
			std::unique_lock<std::mutex> lk(mutex);

			// This locks the mutex (if not already locked) in order to check
			// the predicate (whether the queue contains items). If false, the mutex 
			// is unlocked and waits for the condition variable to receive a signal
			// to check again. If true, code execution continues.
			cv.wait(lk, [this] { return flagUnlock || !dataQueue.empty(); });
			// alt:
			//cv.wait(lk, [&] { return !dataQueue.empty(); });
			// alt:
			//while (dataQueue.empty()) { // include in while loop in case of spurious wakeup
			//	cv.wait(lk); // wait for the condition; wait for notification
			//}

			if (!flagUnlock) {
				// The queue contains elements. Get an element.
				elem = dataQueue.front();
				dataQueue.pop();
			}
		}

		// If an element has been received, process it.
		if (elem) {

			// Process this element
			_process(elem->data.frame);

			// Send out this data
			ofNotifyEvent(*(elem->dataReadyEvent), elem->data);

			// Save the last data frame for reference
			lastFrame = elem->data.frame;

			// Delete this data
			delete elem;
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::exit() {

}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::processAndSend(ofxRTLSEventArgs& data, 
	ofEvent<ofxRTLSEventArgs>& dataReadyEvent) {

	// Create a data element
	DataElem* elem = new DataElem();
	elem->data = data;
	elem->dataReadyEvent = &dataReadyEvent;

	// Add the element to the queue
	{
		std::lock_guard<std::mutex> lk(mutex);
		dataQueue.push(elem);
	}
	// Notify the thread that the condition has been met to proceed
	cv.notify_one();
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process(RTLSProtocol::TrackableFrame& frame) {

	_process_mapIDs(frame);

	_process_removeInvalidIDs(frame);

	_process_applyHungarian(frame);

	_process_applyFilters(frame);
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_mapIDs(RTLSProtocol::TrackableFrame& frame) {
	if (!bMapIDs) return;

	for (int i = 0; i < frame.trackables_size(); i++) {
		int ID = frame.trackables(i).id();
		if (ID < 0) continue;
		frame.mutable_trackables(i)->set_id(dict.lookup(ID));
	}
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_removeInvalidIDs(RTLSProtocol::TrackableFrame& frame) {
	if (!bRemoveInvalidIDs) return;

	// Remove IDs that are invalid (< 0)

	int i = 0;
	while (i < frame.trackables_size()) {

		// Check if this trackable's ID is invalid
		if (frame.trackables(i).id() < 0) {
			// Arbitrary elements cannot be deleted. Swap this element with the last
			// and remove the last element.
			frame.mutable_trackables()->SwapElements(i, frame.trackables_size() - 1);
			frame.mutable_trackables()->RemoveLast();
		}
		else {
			i++;
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_applyHungarian(RTLSProtocol::TrackableFrame& frame) {
	if (!bApplyHungarian) return;

	// Use the hungarian algorithm to attempt to identify continuity across samples.

	// ---------------------------------------
	// ----------- UPDATE MAPPINGS -----------
	// ---------------------------------------

	// Collect data from the previous frame.
	vector<HungarianSample> fromSamples;
	for (int i = 0; i < lastFrame.trackables_size(); i++) {

		TrackableKeyType keyType;
		string key = getTrackableKey(lastFrame.trackables(i), keyType);
		if (isIncludedInHungarianMapping(keyType, hungarianMappingFrom)) {
			// Found a sample in the "FROM" set
			HungarianSample sample;
			sample.key = key;
			sample.index = i;
			sample.position = glm::vec3(
				lastFrame.trackables(i).position().x(),
				lastFrame.trackables(i).position().y(),
				lastFrame.trackables(i).position().z());
			fromSamples.push_back(sample);
		}
	}

	// Collect data from the current frame.
	vector<HungarianSample> toSamples;
	for (int i = 0; i < frame.trackables_size(); i++) {

		TrackableKeyType keyType;
		string key = getTrackableKey(frame.trackables(i), keyType);
		if (isIncludedInHungarianMapping(keyType, hungarianMappingTo)) {
			// Found a sample in the "TO" set
			HungarianSample sample;
			sample.key = key;
			sample.index = i;
			sample.position = glm::vec3(
				frame.trackables(i).position().x(),
				frame.trackables(i).position().y(),
				frame.trackables(i).position().z());
			toSamples.push_back(sample);
		}
	}

	// Solve the assignment problem
	ofxHungarian::solve(fromSamples, toSamples, hungarianRadius);

	// Iterate through all of the TO samples. If the mapped to index is valid, then
	// this point correlates with a sample from the previous frame.
	// TODO: Should the set we use to caluclate mappings be different than the 
	// set for which mappings are changed? (e.g. if both mappings are temporary, we are
	// throwing away a-priori information about permanent, given points that may influence 
	// the assignment)?
	for (auto& toSample : toSamples) {
		if (toSample.mapTo < 0) continue;

		// Carry over the tracking (making it continuous) by recording a new mapping from 
		// one ID (filter key) to another ID (filter key).

		// Map the TO ID (which doesn't exist in filters) to the 
		// FROM ID (which already exists in filters).
		string newKey = toSample.key;
		string existingKey = fromSamples[toSample.mapTo].key;

		// Check if the existing key is already in the mappings. If so, find the last key
		// in the chain.
		// This shouldn't happen, but let's do it anyway (stop if overflow).
		int counter = keyMappingRecursionLimit;
		while (counter > 0 && keyMappings.find(existingKey) != keyMappings.end()) {
			existingKey = keyMappings[existingKey];
			counter--;
		}
		if (counter == 0) {
			ofLogError("ofxRTLSPostprocessor") << "Hungarian algorithm key mapping depth exceeded recursion limit of " << keyMappingRecursionLimit;
		}

		// Set the new key mapping
		keyMappings[newKey] = existingKey;
	}

	// ---------------------------------------
	// ----------- APPLY MAPPINGS ------------
	// ---------------------------------------

	// Apply the mappings to the trackables
	
	// Iterate through all TO samples
	for (auto& sample : toSamples) {
		if (sample.mapTo < 0) continue; // no mapping found

		// A Mapping has been found for this trackable. Set the new identifiable information.
		reconcileTrackableWithKey(*(frame.mutable_trackables(sample.index)), keyMappings[sample.key]);
	}
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_applyFilters(RTLSProtocol::TrackableFrame& frame) {
	if (!bApplyFilters) return;

	// For all that remain in the filter, set their new coordinates and export them

	// Input the new data
	for (int i = 0; i < frame.trackables_size(); i++) {
		// Add new data to the filter 
		glm::vec3 position = glm::vec3(
			frame.trackables(i).position().x(),
			frame.trackables(i).position().y(),
			frame.trackables(i).position().z());
		auto* filter = filters.getFilter(getTrackableKey(frame.trackables(i)));
		filter->process(position);
	}

	// Process any remaining filters that haven't seen data
	filters.processRemaining();

	// Delete any data that is invalid.
	// Also save IDs for all data that is valid.
	set<string> existingDataIDs;
	int i = 0;
	while (i < frame.trackables_size()) {
		// Check if this trackable's data is invalid.
		ofxFilter* filter = filters.getFilter(getTrackableKey(frame.trackables(i)));
		if (!filter->isDataValid()) {
			// If not, delete it
			frame.mutable_trackables()->SwapElements(i, frame.trackables_size() - 1);
			frame.mutable_trackables()->RemoveLast();
		}
		else {
			// Save that this ID has valid data
			existingDataIDs.insert(getTrackableKey(frame.trackables(i)));

			// Set this new processed data
			glm::vec3 data = filter->getPosition();
			frame.mutable_trackables(i)->mutable_position()->set_x(data.x);
			frame.mutable_trackables(i)->mutable_position()->set_y(data.y);
			frame.mutable_trackables(i)->mutable_position()->set_z(data.z);

			i++;
		}
	}

	// Add any data that isn't present
	for (auto& it : filters.getFilters()) {
		// Check if this is a new ID and if it has valid data.
		if (existingDataIDs.find(it.first) == existingDataIDs.end() && it.second->isDataValid()) {
			// If so, add a trackable			
			Trackable* trackable = frame.add_trackables();
			// Set the identifiable information of this trackable
			reconcileTrackableWithKey(*trackable, it.first);
			// Set the position
			Trackable::Position* position = trackable->mutable_position();
			glm::vec3 data = it.second->getPosition();
			position->set_x(data.x);
			position->set_y(data.y);
			position->set_z(data.z);
		}
	}

	// Delete any filters that haven't been used recently
	if (ofGetElapsedTimeMillis() - lastFilterCullingTime > filterCullingPeriod) {
		lastFilterCullingTime = ofGetElapsedTimeMillis();
		filters.removeUnused();
	}
}

// --------------------------------------------------------------
string ofxRTLSPostprocessor::getTrackableKey(const Trackable& t) {

	TrackableKeyType keyType = getTrackableKeyType(t);
	return getTrackableKey(t, keyType);
}

// --------------------------------------------------------------
string ofxRTLSPostprocessor::getTrackableKey(const Trackable& t, TrackableKeyType& keyType) {

	keyType = getTrackableKeyType(t);
	string prefix = ofToString(int(keyType));
	string data = "";
	switch (keyType) {
	case KEY_ID: {
		data = ofToString(t.id());
	}; break;
	case KEY_CUID: {
		data = t.cuid();
	}; break;
	case KEY_NAME: {
		data = t.name();
	}; break;
	case KEY_INVALID: default: {
		data = "";
	}
	}
	return prefix + data;
}

// --------------------------------------------------------------
ofxRTLSPostprocessor::TrackableKeyType ofxRTLSPostprocessor::getTrackableKeyType(const Trackable& t) {

	if (t.id() > 0) return KEY_ID;
	if (!t.cuid().empty()) return KEY_CUID;
	if (!t.name().empty()) return KEY_NAME;
	// If an ID is 0, it will be marked as invalid.
	return KEY_INVALID;
}

// --------------------------------------------------------------
ofxRTLSPostprocessor::TrackableKeyType ofxRTLSPostprocessor::getTrackableKeyType(string key) {
	if (key.empty()) return KEY_INVALID;

	if (key[0] == '0') return KEY_INVALID;
	if (key[0] == '1') return KEY_ID;
	if (key[0] == '2') return KEY_CUID;
	if (key[0] == '3') return KEY_NAME;
	return KEY_INVALID;
}

// --------------------------------------------------------------
string ofxRTLSPostprocessor::getTrackableKeyData(string key) {

	if (key.empty()) return "";
	return key.substr(1, string::npos);
}

// --------------------------------------------------------------
string ofxRTLSPostprocessor::getTrackableKeyTypeDescription(TrackableKeyType keyType) {

	switch (keyType) {
	case KEY_ID: return "id";
	case KEY_CUID: return "cuid";
	case KEY_NAME: return "name";
	case KEY_INVALID: default: return "invalid";
	}
}

// --------------------------------------------------------------
bool ofxRTLSPostprocessor::reconcileTrackableWithKey(Trackable& t, string key) {

	// Get the string's key type
	TrackableKeyType keyType = getTrackableKeyType(t);
	if (keyType == KEY_INVALID) return false;
	string data = getTrackableKeyData(key);

	// Clear ID. Only set this if the ID is the identifiable data source.
	t.clear_id();
	if (keyType == KEY_ID) {
		t.set_id(ofToInt(data));
		return true;
	}
	
	// Clear CUID. Only set this if CUID is the identifiable data source.
	t.clear_cuid();
	if (keyType == KEY_CUID) {
		t.set_cuid(data);
		return true;
	}

	t.clear_name();
	if (keyType == KEY_NAME) {
		t.set_name(data);
		return true;
	}

	// should not get to this point
	return false;
}

// --------------------------------------------------------------
bool ofxRTLSPostprocessor::isIncludedInHungarianMapping(TrackableKeyType keyType, HungarianMapping mapping) {

	if (mapping == HungarianMapping::TEMPORARY || mapping == HungarianMapping::TEMPORARY_AND_PERMANENT) {
		if (tempKeyTypes.find(keyType) != tempKeyTypes.end()) return true;
	}

	if (mapping == HungarianMapping::PERMANENT || mapping == HungarianMapping::TEMPORARY_AND_PERMANENT) {
		if (permKeyTypes.find(keyType) != permKeyTypes.end()) return true;
	}

	return false;
}

// --------------------------------------------------------------

// --------------------------------------------------------------



#endif