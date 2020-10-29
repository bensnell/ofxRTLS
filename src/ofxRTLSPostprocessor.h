#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;
#include "ofxRTLSTrackableKey.h"

#ifdef RTLS_POSTPROCESS

#include "IDDictionary.h"
#include "ofxFDeep.h"
#include "ofxFilterGroup.h"
#include "ofxHungarian.h"
#include "ofxRTLSTypes.h"

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

class CuidGenerator {
public:
	CuidGenerator(uint64_t startCounter) { counter = startCounter; }
	~CuidGenerator() {};
	uint64_t getNewCuid() { counter++; return counter; }
private:
	uint64_t counter = 0;
};

class ofxRTLSPostprocessor : public ofThread {
public:

	// Create an object postprocess data.
	ofxRTLSPostprocessor();
	~ofxRTLSPostprocessor();

	// Setup this postprocessor.
	void setup(RTLSSystemType systemType, RTLSTrackableType trackableType, string name, string abbr, string dictPath="", string filterList="");

	// Process data and send it when ready
	void processAndSend(ofxRTLSEventArgs& data, ofEvent<ofxRTLSEventArgs>& dataReadyEvent);

	// Reset the postprocessor.
	// (This will reset the internal states, removing histories.)
	// (This does not delete existing configurations.)
	void reset();

	void exit();

	void resetEventReceved(ofxRTLSPlayerLoopedArgs& args);

private:

	// Postprocessor Parameters
	RTLSSystemType systemType = RTLS_SYSTEM_TYPE_INVALID;
	RTLSTrackableType trackableType = RTLS_TRACKABLE_TYPE_INVALID;
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
	void _process_applyHungarian(RTLSProtocol::TrackableFrame& frame);
	void _process_removeUnidentifiable(RTLSProtocol::TrackableFrame& frame);
	void _process_applyFilters(RTLSProtocol::TrackableFrame& frame);
	bool bMapIDs = true;
	bool bRemoveUnidentifiableBeforeHungarian = true;
	bool bApplyHungarian = true;
	bool bRemoveUnidentifiableBeforeFilters = true;
	bool bApplyFilters = true;

	RTLSProtocol::TrackableFrame lastFrame;

	// Dictionary for mapping IDs
	IDDictionary dict;
	string dictPath = "";


	// Hungarian Algorithm-related Parameters
	// Temporary and Permanently Identifiable Fields
	string tempKeyTypesStr = "cuid,none";	// key types that may change per trackable
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
	// Before solving, should samples with keys that appear in both the FROM and 
	// TO sets be removed? This likely speeds up the solver by removing superfluous
	// samples for which there are already matches.
	bool bRemoveMatchingKeysBeforeSolve = true;
	// Generator of cuids for unidentifiable points
	bool bAssignCuidsToUnidentifiableTrackables = false;
	// What number should the counter start at?
	int cuidStartCounter = 80000;
	CuidGenerator* cuidGen;
	// Are remappings from permanent key types allowed?
	// (If so, it is highly recommended that you remove matching keys before solve, 
	// since this will prevent permanent IDs from separating during tracking.)
	// For example, what if a trackable with a permanent ID suddenly
	// loses identification and is now identifiable only by a temporary CUID?
	bool bAllowRemappingFromPermKeyTypes = false;
	// Are remappings to permanent key types allowed?
	// For example, what if a trackable with a temporary CUID suddently turns
	// into an identifiable trackable with a permanent ID?
	bool bAllowRemappingToPermKeyTypes = false;


	// Filters for smoothing data, etc.
	ofxFilterGroup filters;
	// When was the last time filters were culled? (ms)
	uint64_t lastFilterCullingTime = 0;
	// What is the period by which filters are culled? (ms)
	uint64_t filterCullingPeriod = 1000; // each second

	atomic<bool> flagReset = false;
	void resetInternalStates();
};

#endif