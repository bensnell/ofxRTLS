#include "IDDictionary.h"

// --------------------------------------------------------------
IDDictionary::IDDictionary() {
}

// --------------------------------------------------------------
IDDictionary::~IDDictionary() {
}

// --------------------------------------------------------------
void IDDictionary::setup(string _filepath) {

	// Load the settings
	RUI_NEW_GROUP("ID Dictionary");
	RUI_SHARE_PARAM_WCN("IDDict- Filepath", filepath);

	// Attempt to load from file
	if (!_filepath.empty()) filepath = _filepath;
	RUI_PUSH_TO_CLIENT();

	// Attempt to load the file
	ofFile file(ofToDataPath(filepath));
	if (!file.exists()) {
		ofLogError("IDDictionary") << "Dictionary file appears to be missing: " << filepath;
		return;
	}
	ofJson js;
	file >> js;
	if (js.find("nBits") == js.end()) {
		ofLogError("IDDictionary") << "JSON must contain a variable \"nBits\"";
		return;
	}
	int nBits = js["nBits"];
	if (js.find("dict") == js.end()) {
		ofLogError("IDDictionary") << "JSON dictionary must be mapped to key \"dict\"";
		return;
	}
	if (js["dict"].size() != pow(2, nBits)) {
		ofLogError("IDDictionary") << "Dict must contain all values, even invalid ones";
		return;
	}
	dict.resize(pow(2, nBits), -1);
	for (int i = 0; i < dict.size(); i++) {
		dict[i] = js["dict"][i];
	}
	ofLogNotice("IDDictionary") << "Dictionary successfully loaded from " << filepath;
}

// --------------------------------------------------------------
int IDDictionary::lookup(int value) {
	if (!bEnabled) return value;

	if (dict.empty() || value < 0 || value > dict.size()) return -1;
	else return dict[value];
}

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------
