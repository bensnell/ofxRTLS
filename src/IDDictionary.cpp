#include "IDDictionary.h"

// --------------------------------------------------------------
IDDictionary::IDDictionary() {
}

// --------------------------------------------------------------
IDDictionary::~IDDictionary() {
}

// --------------------------------------------------------------
bool IDDictionary::setup(string _filepath) {
	bValid = false;
	dict.clear();

	// Attempt to load from file
	if (_filepath.empty()) {
		ofLogError("IDDictionary") << "Dictionary path is empty. Cannot load from file.";
		return bValid;
	}
	filepath = _filepath;

	// Attempt to load the file
	ofFile file(ofToDataPath(filepath));
	if (!file.exists()) {
		ofLogError("IDDictionary") << "Dictionary file appears to be missing: " << filepath;
		return bValid;
	}
	ofJson js;
	file >> js;
	if (js.find("nBits") == js.end()) {
		ofLogError("IDDictionary") << "JSON must contain a variable \"nBits\"";
		return bValid;
	}
	int nBits = js["nBits"];
	if (js.find("dict") == js.end()) {
		ofLogError("IDDictionary") << "JSON dictionary must be mapped to key \"dict\"";
		return bValid;
	}
	if (js["dict"].size() != pow(2, nBits)) {
		ofLogError("IDDictionary") << "Dict must contain all values, even invalid ones";
		return bValid;
	}
	dict.resize(pow(2, nBits), -1);
	for (int i = 0; i < dict.size(); i++) {
		dict[i] = js["dict"][i];
	}
	ofLogNotice("IDDictionary") << "Dictionary successfully loaded from " << filepath;
	bValid = true;
	return bValid;
}

// --------------------------------------------------------------
int IDDictionary::lookup(int value) {
	if (!bValid) return value;

	if (dict.empty() || value < 0 || value > dict.size()) return -1;
	else return dict[value];
}

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------
