#include "ofxRTLSTake.h"

#ifdef RTLS_PLAYER

// --------------------------------------------------------------
bool RTLSPlayerTake::populateTemplateFrames() {
	if (c3d == NULL) return false;

	// Clear any existing data
	frames.clear();
	dataIndexToFrameIndex.clear();

	// Get identifiable info for all points
	vector<string> labels(c3d->parameters().group("POINT").parameter("LABELS").valuesAsString());
	vector<string> descriptions(c3d->parameters().group("POINT").parameter("DESCRIPTIONS").valuesAsString());
	assert(labels.size() == descriptions.size());

	// Parse all descriptions
	vector<ofJson> contexts;
	for (auto& d : descriptions) {
		// TODO: try/catch?
		ofJson js = ofJson::parse(d);
		contexts.push_back(js);
	}

	// Round up all possible system and trackable type combinations.
	// Create trackable frame templates with appropriate types.
	vector<string> configs;
	for (int i = 0; i < contexts.size(); i++) {
		auto& js = contexts[i];
		if (js.find("frame") == js.end() ||
			js["frame"].find("context") == js["frame"].end() ||
			js["frame"]["context"].find("s") == js["frame"]["context"].end() ||
			js["frame"]["context"].find("t") == js["frame"]["context"].end()) {
			ofLogError("ofxRTLSTake") << "Take must contain frame-context-s and frame-context-t";
			return false;
		}
		int s = js["frame"]["context"]["s"];
		int t = js["frame"]["context"]["t"];
		string key = ofToString(s) + "_" + ofToString(t);
		auto it = find(configs.begin(), configs.end(), key);
		if (it == configs.end()) { // No match

			// Store this match
			configs.push_back(key);

			// Create a new frame
			frames.push_back(Frame());
			frames.back().frame.set_context(js["frame"]["context"].dump());
			frames.back().systemType = RTLSSystemType(s);
			frames.back().trackableType = RTLSTrackableType(t);

			// Store the index to maps
			frames.back().dataIndices.push_back(i);
			dataIndexToFrameIndex[i] = frames.size()-1;
		}
		else { // Match
			int index = (it - configs.begin());

			// Store the index to maps
			frames[index].dataIndices.push_back(i);
			dataIndexToFrameIndex[i] = index;
		}
	}

	// Get a list of all trackable identifiable properties
	vector<string> allKeyTypes = getTrackableKeyTypeDescriptionAll();

	// Fill the frame templates with data from each trackable
	for (int i = 0; i < contexts.size(); i++) {
		
		// Get identifiable data for this trackable
		auto& js = contexts[i];
		string key = labels[i];

		// Get the frame
		int frameIndex = dataIndexToFrameIndex[i];
		Frame& f = frames[frameIndex];

		// Create a trackable
		Trackable* tk = f.frame.add_trackables();
		// Set the identifiable property of this trackable
		if (!reconcileTrackableWithKey(*tk, key)) {
			ofLogError("ofxRTLSPlayer") << "Could not reconcile trackable with key.";
			return false; // ?
		}
		// Set all remaining properties
		if (js.find("trackable") == js.end()) {
			ofLogError("ofxRTLSTake") << "There must be a trackable field.";
			return false;
		}
		for (int i = 0; i < allKeyTypes.size(); i++) {
			if (js["trackable"].find(allKeyTypes[i]) != js["trackable"].end()) {
				if (allKeyTypes[i].compare("id") == 0) {
					int id = js["trackable"]["id"];
					tk->set_id(id);
				}
				if (allKeyTypes[i].compare("cuid") == 0) {
					string cuid = js["trackable"]["cuid"];
					tk->set_cuid(cuid);
				}
				if (allKeyTypes[i].compare("name") == 0) {
					string name = js["trackable"]["name"];
					tk->set_name(name);
				}
			}
		}
		// Set the context
		if (js["trackable"].find("context") != js["trackable"].end()) {
			tk->set_context(js["trackable"]["context"].dump());
		}
	}

	return true;
}

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

#endif