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
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Remove UnID Before Hungarian", bRemoveUnidentifiableBeforeHungarian);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Apply Hungarian", bApplyHungarian);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Remove UnID Before Filters", bRemoveUnidentifiableBeforeFilters);
	RUI_SHARE_PARAM_WCN(ruiGroupAbbr + "- Apply Filters", bApplyFilters);
	
	// Setup the dictionary params
	RUI_NEW_GROUP("IDDictionary - " + abbr);
	RUI_SHARE_PARAM_WCN("ID_RTLS" + abbr + "- ID Dict Path", dictPath);

	// Load the dictionary
	if (!dictPath.empty()) dict.setup(dictPath);

	// Setup the hungarian algorithm
	RUI_NEW_GROUP("Hungarian - " + abbr);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Temporary Key Types", tempKeyTypesStr);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Permanent Key Types", permKeyTypesStr);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Item Radius", hungarianRadius, 0, 1000000);
	vector<string> hungarianMappings = { "Temporary", "Permanent", "Both" };
	RUI_SHARE_ENUM_PARAM_WCN("HU_RTLS" + abbr + "- From Dataset Permanence", hungarianMappingFrom, HungarianMapping::TEMPORARY, HungarianMapping::TEMPORARY_AND_PERMANENT, hungarianMappings);
	RUI_SHARE_ENUM_PARAM_WCN("HU_RTLS" + abbr + "- To Dataset Permanence", hungarianMappingTo, HungarianMapping::TEMPORARY, HungarianMapping::TEMPORARY_AND_PERMANENT, hungarianMappings);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Map Recursion Limit", keyMappingRecursionLimit, 0, 10000);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Remove Matching Keys", bRemoveMatchingKeysBeforeSolve);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Assign CUIDs to UnID", bAssignCuidsToUnidentifiableTrackables);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- CUID Start Counter", cuidStartCounter, 0, 1000000);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Allow Remap From Perm", bAllowRemappingFromPermKeyTypes);
	RUI_SHARE_PARAM_WCN("HU_RTLS" + abbr + "- Allow Remap To Perm", bAllowRemappingToPermKeyTypes);


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

	// Setup the CUID generator
	cuidGen = new CuidGenerator(cuidStartCounter);

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

	if (bMapIDs) _process_mapIDs(frame);

	if (bRemoveUnidentifiableBeforeHungarian) _process_removeUnidentifiable(frame);

	if (bApplyHungarian) _process_applyHungarian(frame);

	if (bRemoveUnidentifiableBeforeFilters) _process_removeUnidentifiable(frame);

	if (bApplyFilters) _process_applyFilters(frame);
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_mapIDs(RTLSProtocol::TrackableFrame& frame) {

	for (int i = 0; i < frame.trackables_size(); i++) {
		if (!isTrackableIdentifiableByType(frame.trackables(i), KEY_ID) || 
			!isTrackableIDValid(frame.trackables(i))) continue;

		int ID = frame.trackables(i).id();
		frame.mutable_trackables(i)->set_id(dict.lookup(ID));
	}
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_removeUnidentifiable(RTLSProtocol::TrackableFrame& frame) {

	int i = 0;
	while (i < frame.trackables_size()) {

		// Check if this trackable can be identified
		if (!isTrackableIdentifiable(frame.trackables(i))) {
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

	// Use the hungarian algorithm to attempt to identify continuity across samples.

	// ---------------------------------------
	// ------- APPLY EXISTING MAPPINGS -------
	// ---------------------------------------

	// First, apply existing mappings to the current frame
	for (int i = 0; i < frame.trackables_size(); i++) {
		string key = getTrackableKey(frame.trackables(i));
		if (keyMappings.find(key) != keyMappings.end()) {
			// found a mapping
			reconcileTrackableWithKey(*frame.mutable_trackables(i), keyMappings[key]);
		}
	}


	// ---------------------------------------
	// -------- COLLECT SOLVER DATA ----------
	// ---------------------------------------

	// Collect data that will be passed to the solver.

	// Optionally (and by recommendation) remove matching keys from this dataset
	set<string> matchingKeys;
	if (bRemoveMatchingKeysBeforeSolve) {

		// Collect keys
		vector<string> fromKeys;
		for (int i = 0; i < lastFrame.trackables_size(); i++) {
			if (!isTrackableIdentifiable(lastFrame.trackables(i))) continue;
			fromKeys.push_back(getTrackableKey(lastFrame.trackables(i)));
		}
		vector<string> toKeys;
		for (int i = 0; i < frame.trackables_size(); i++) {
			if (!isTrackableIdentifiable(frame.trackables(i))) continue;
			toKeys.push_back(getTrackableKey(frame.trackables(i)));
		}

		// Sort keys
		sort(fromKeys.begin(), fromKeys.end());
		sort(toKeys.begin(), toKeys.end());

		// Find the union of both sorted sets
		vector<string> _matchingKeys(fromKeys.size() + toKeys.size());
		vector<string>::iterator it;
		it = set_intersection(fromKeys.begin(), fromKeys.end(), toKeys.begin(), toKeys.end(), _matchingKeys.begin());
		_matchingKeys.resize(it - _matchingKeys.begin());

		// Place this data in a set for quicker lookup
		for (auto& s : _matchingKeys) {
			matchingKeys.insert(s);
		}
	}

	// Collect data from the previous frame.
	vector<HungarianSample> fromSamples;
	for (int i = 0; i < lastFrame.trackables_size(); i++) {

		TrackableKeyType keyType;
		string key = getTrackableKey(lastFrame.trackables(i), keyType);

		// Confirm that this sample should be in the dataset which the solver operates on.
		if ((bRemoveMatchingKeysBeforeSolve && matchingKeys.find(key) != matchingKeys.end())) continue;
		if (!isIncludedInHungarianMapping(keyType, hungarianMappingFrom)) continue;

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

	// Collect data from the current frame.
	vector<HungarianSample> toSamples;
	for (int i = 0; i < frame.trackables_size(); i++) {

		TrackableKeyType keyType;
		string key = getTrackableKey(frame.trackables(i), keyType);

		// Confirm that this sample should be in the dataset which the solver operates on.
		if ((bRemoveMatchingKeysBeforeSolve && matchingKeys.find(key) != matchingKeys.end())) continue;
		if (!isIncludedInHungarianMapping(keyType, hungarianMappingTo)) continue;

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

	// TODO: Would this work if no IDs were provided?
	// If a marker is unidentifiable, include it.


	// ---------------------------------------
	// ------- SOLVE FOR NEW MAPPINGS --------
	// ---------------------------------------

	// Solve the assignment problem
	ofxHungarian::solve(fromSamples, toSamples, hungarianRadius);


	// ---------------------------------------
	// --- COLLECT AND APPLY NEW MAPPINGS ----
	// ---------------------------------------

	// Iterate through all of the TO samples. If the mapped to index is valid, then
	// this point correlates with a sample from the previous frame.
	// TODO: Should the set we use to caluclate mappings be different than the 
	// set for which mappings are changed? (e.g. if both mappings are temporary, we are
	// throwing away a-priori information about permanent, given points that may influence 
	// the assignment)?
	for (auto& toSample : toSamples) {

		// If this sample's key is permanent, then don't remap or reconcile it.
		if (!bAllowRemappingToPermKeyTypes &&
			permKeyTypes.find(getTrackableKeyType(toSample.key)) != permKeyTypes.end()) {
			continue;
		}

		if (toSample.validMapping()) {

			// Carry over the tracking (making it continuous) by recording a new mapping from 
			// one ID (filter key) to another ID (filter key).

			// Map the TO ID (which doesn't exist in filters) to the 
			// FROM ID (which already exists in filters).
			string newKey = toSample.key;
			string existingKey = fromSamples[toSample.mapTo].key;

			// If the FROM trackable was not identifiable, then there's nothing
			// to map to, so skip.
			if (!isTrackableIdentifiable(existingKey)) continue;

			// If the keys are the same, then skip.
			// Always remove matching keys after the solve.
			if (newKey.compare(existingKey) == 0) continue;

			// Make sure the existing key is at the lowest depth possible.
			if (isTrackableIdentifiable(newKey)) {

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
			}

			// If we're mapping from a permanent key type, we may need to skip
			if (!bAllowRemappingFromPermKeyTypes &&
				permKeyTypes.find(getTrackableKeyType(existingKey)) != permKeyTypes.end()) {
				continue;
			}

			// If the new key is identifiable, add a new mapping.
			if (isTrackableIdentifiable(newKey)) keyMappings[newKey] = existingKey;

			// Reconcile the trackable by setting its new information. Set the new identifiable information.
			reconcileTrackableWithKey(*(frame.mutable_trackables(toSample.index)), existingKey);

		}
		else {
			// Invalid mapping...

			// ... If this sample is unidentifiable and we're assigning CUIDs, then give it a CUID.
			if (bAssignCuidsToUnidentifiableTrackables && !isTrackableIdentifiable(toSample.key)) {
			
				// Create a CUID
				uint64_t cuid = cuidGen->getNewCuid();

				// Create a key
				string key = getTrackableKey(KEY_CUID, ofToString(cuid));

				// Reconcile
				reconcileTrackableWithKey(*(frame.mutable_trackables(toSample.index)), key);
			}
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSPostprocessor::_process_applyFilters(RTLSProtocol::TrackableFrame& frame) {

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

// --------------------------------------------------------------

#endif