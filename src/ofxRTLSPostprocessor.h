#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

#ifdef RTLS_POSTPROCESS

#include "IDDictionary.h"
#include "ofxFDeep.h"
#include "ofxFilterGroup.h"
#include "ofxHungarian.h"

// Locking with Condition Variables, Queues and Mutex follows the 
// examples set forth here:
// https://ncona.com/2019/04/using-condition-variables-in-cpp/
// Also useful:
// https://wiki.sei.cmu.edu/confluence/display/cplusplus/CON54-CPP.+Wrap+functions+that+can+spuriously+wake+up+in+a+loop
// https://en.cppreference.com/w/cpp/thread/condition_variable
// https://www.justsoftwaresolutions.co.uk/threading/multithreading-in-c++0x-part-7-locking-multiple-mutexes.html
// https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables
// http://jakascorner.com/blog/2016/02/lock_guard-and-unique_lock.html

// Note: The RTLS Protocol by default places zero in empty fields. Therefore,
// an ID of 0 is not supported by the postprocessor. It will be treated as
// an invalid value.

class ofxRTLSPostprocessor : public ofThread {
public:

	// Create an object postprocess data.
	ofxRTLSPostprocessor();
	~ofxRTLSPostprocessor();

	// Setup this postprocessor.
	void setup(string name, string abbr, string dictPath="", string filterList="");

	// Process data and send it when ready
	void processAndSend(ofxRTLSEventArgs& data, ofEvent<ofxRTLSEventArgs>& dataReadyEvent);

	void exit();

private:

	// Postprocessor Parameters
	string name = "";
	string abbr = "";

	void threadedFunction();
	std::condition_variable cv;
	atomic<bool> flagUnlock = false;

	// Queue holds data ready to be processed
	struct DataElem {
		ofxRTLSEventArgs data;
		ofEvent<ofxRTLSEventArgs>* dataReadyEvent;
	};
	queue< DataElem* > dataQueue;

	// Process a data element
	void _process(RTLSProtocol::TrackableFrame& frame);
	void _process_mapIDs(RTLSProtocol::TrackableFrame& frame);
	void _process_removeInvalidIDs(RTLSProtocol::TrackableFrame& frame);
	void _process_applyHungarian(RTLSProtocol::TrackableFrame& frame);
	void _process_applyFilters(RTLSProtocol::TrackableFrame& frame);
	bool bMapIDs = true;
	bool bRemoveInvalidIDs = true;
	bool bApplyHungarian = true;
	bool bApplyFilters = true;

	RTLSProtocol::TrackableFrame lastFrame;

	// Dictionary for mapping IDs
	IDDictionary dict;
	string dictPath = "";

	// In order to identify trackables, each trackable must have a unique key.
	// Note: The trackable ID cannot be zero.
	// Keys consist of two pieces of information:
	//	Character 0 ("Prefix"):
	//		TrackableKeyType
	enum TrackableKeyType {
		KEY_INVALID = 0,
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
	// This function will return the type of the key.
	TrackableKeyType getTrackableKeyType(const Trackable& t);
	TrackableKeyType getTrackableKeyType(string key);
	// From a given key, get the raw information that correlates with the type.
	// For example, if key = "112", then the type is ID and the ID is "12" as a string.
	string getTrackableKeyData(string key);
	// Get the English description of this key type
	string getTrackableKeyTypeDescription(TrackableKeyType keyType);
	// Reconcile a trackable with its trackable key. Align the trackable's 
	// internal information so that the key would be correct.
	bool reconcileTrackableWithKey(Trackable& t, string key);




	// Hungarian Algorithm-related Parameters
	// Temporary and Permanently Identifiable Fields
	string tempKeyTypesStr = "cuid";	// key types that may change per trackable
	string permKeyTypesStr = "id,name";	// key types that will not change per trackable
	set<TrackableKeyType> tempKeyTypes;
	set<TrackableKeyType> permKeyTypes;
	// Radius of the items (for calculating intersection)
	// (If items are farther apart then this, no intersection is calculated and 
	// the item cannot be tracked).
	float hungarianRadius = 0.1;
	// How are samples mapped?
	enum HungarianMapping {
		TEMPORARY = 0,
		PERMANENT,
		TEMPORARY_AND_PERMANENT
	};
	HungarianMapping hungarianMappingFrom = TEMPORARY;
	HungarianMapping hungarianMappingTo = TEMPORARY;
	bool isIncludedInHungarianMapping(TrackableKeyType keyType, HungarianMapping mapping);
	// These key mappings are the byproduct of the hungarian algorithm, and
	// should be applied in the step before filtering.
	map<string, string> keyMappings;
	// What is the recursion limit of the mappings? (for safety)
	int keyMappingRecursionLimit = 100;





	// Filters for smoothing data, etc.
	ofxFilterGroup filters;
	// When was the last time filters were culled? (ms)
	uint64_t lastFilterCullingTime = 0;
	// What is the period by which filters are culled? (ms)
	uint64_t filterCullingPeriod = 1000; // each second
};

#endif