#include "ofxRTLSTrackableKey.h"

// --------------------------------------------------------------
string getTrackableKey(const Trackable& t) {

	TrackableKeyType keyType;
	return getTrackableKey(t, keyType);
}

// --------------------------------------------------------------
string getTrackableKey(const Trackable& t, TrackableKeyType& keyType) {

	keyType = getTrackableKeyType(t);
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
	case KEY_NONE: default: {
		data = "";
	}
	}
	return getTrackableKey(keyType, data);
}

// --------------------------------------------------------------
string getTrackableKey(TrackableKeyType keyType, string data) {

	string prefix = ofToString(int(keyType));
	return prefix + data;
}

// --------------------------------------------------------------
TrackableKeyType getTrackableKeyType(const Trackable& t) {

	// Unidentifiable trackables have negative IDs
	if (isTrackableFlaggedUnidentifiable(t)) return KEY_NONE;
	// An ID of 0 indicates that the trackable, if identifiable,
	// is not identifiable via the ID parameter, but might be
	// identifiable via another parameter.
	if (isTrackableIDValid(t)) return KEY_ID;
	if (!t.cuid().empty()) return KEY_CUID;
	if (!t.name().empty()) return KEY_NAME;
	// If an ID is 0, the key will be marked as none.
	return KEY_NONE;
}

// --------------------------------------------------------------
TrackableKeyType getTrackableKeyType(string key) {
	if (key.empty()) return KEY_NONE;

	if (key[0] == '0') return KEY_NONE;
	if (key[0] == '1') return KEY_ID;
	if (key[0] == '2') return KEY_CUID;
	if (key[0] == '3') return KEY_NAME;
	return KEY_NONE;
}

// --------------------------------------------------------------
string getTrackableKeyData(string key) {

	if (key.empty()) return "";
	return key.substr(1, string::npos);
}

// --------------------------------------------------------------
string getTrackableKeyTypeDescription(TrackableKeyType keyType) {

	switch (keyType) {
	case KEY_ID: return "id";
	case KEY_CUID: return "cuid";
	case KEY_NAME: return "name";
	case KEY_NONE: default: return "none";
	}
}

// --------------------------------------------------------------
bool reconcileTrackableWithKey(Trackable& t, string key) {

	// Get the destination's key type
	TrackableKeyType keyType = getTrackableKeyType(key);
	//if (keyType == KEY_NONE) return false;
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
bool isTrackableIdentifiable(const Trackable& t) {

	return isTrackableIdentifiable(getTrackableKeyType(t));
}

// --------------------------------------------------------------
bool isTrackableIdentifiable(string& key) {

	return isTrackableIdentifiable(getTrackableKeyType(key));
}

// --------------------------------------------------------------
bool isTrackableIdentifiable(TrackableKeyType keyType) {

	return keyType != KEY_NONE;
}

// --------------------------------------------------------------
bool isTrackableIDValid(const Trackable& t) {

	return t.id() > 0;
}

// --------------------------------------------------------------
bool isTrackableIDEmpty(const Trackable& t) {

	return t.id() == 0;
}

// --------------------------------------------------------------
bool isTrackableFlaggedUnidentifiable(const Trackable& t) {

	return t.id() < 0;
}

// --------------------------------------------------------------
bool isTrackableIdentifiableByType(const Trackable& t, TrackableKeyType keyType) {

	return getTrackableKeyType(t) == keyType;
}

// --------------------------------------------------------------
vector<string> getTrackableKeyTypeDescriptionAll() {

	vector<string> out;
	for (int i = 0; i < (int(TrackableKeyType::NUM_KEYS) - 1); i++) {
		TrackableKeyType type = TrackableKeyType(i);
		out.push_back(getTrackableKeyTypeDescription(type));
	}
	return out;
}

// --------------------------------------------------------------

// --------------------------------------------------------------
