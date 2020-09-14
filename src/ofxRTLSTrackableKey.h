#pragma once

#include "ofMain.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;


// In order to identify trackables, each trackable must have a unique key.
// Note: The trackable ID cannot be zero.
// Keys consist of two pieces of information:
//	Character 0 ("Prefix"):
//		TrackableKeyType
enum TrackableKeyType {
	KEY_NONE = 0,
	KEY_ID,			// integer
	KEY_CUID,		// 16 digit string, representing two uint64_t values
	KEY_NAME,		// string, any length
	NUM_KEYS
};

//	Characters 1+ ("Data"):
//		string of variable length that is parsed according to the Prefix
// This function will return the key used to track each point.
string getTrackableKey(const Trackable& t);
string getTrackableKey(const Trackable& t, TrackableKeyType& keyType);
string getTrackableKey(TrackableKeyType keyType, string data);

// This function will return the type of the key.
TrackableKeyType getTrackableKeyType(const Trackable& t);
TrackableKeyType getTrackableKeyType(string key);
	
// From a given key, get the raw information that correlates with the type.
// For example, if key = "112", then the type is ID and the ID is "12" as a string.
string getTrackableKeyData(string key);
	
// Get the English description of this key type
string getTrackableKeyTypeDescription(TrackableKeyType keyType);
vector<string> getTrackableKeyTypeDescriptionAll();
	
// Reconcile a trackable with its trackable key. Align the trackable's 
// internal information so that the key would be correct.
bool reconcileTrackableWithKey(Trackable& t, string key);
	
// Is a trackable unidentifiable (not possessing a valid key)? 
bool isTrackableIdentifiable(const Trackable& t);
bool isTrackableIdentifiable(string& key);
bool isTrackableIdentifiable(TrackableKeyType keyType);

// Is a trackable identifiable by a specific type?
bool isTrackableIdentifiableByType(const Trackable& t, TrackableKeyType keyType);

// Is a trackable identifiable by ID valid?
bool isTrackableIDValid(const Trackable& t);
