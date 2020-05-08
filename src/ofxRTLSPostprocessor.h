#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

#ifdef RTLS_ENABLE_POSTPROCESS

#include "IDDictionary.h"
#include "ofxFDeep.h"
#include "ofxFilterGroup.h"

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

	// Toggles
	bool bMapIDs = true;
	bool bRemoveInvalidIDs = true;
	bool bApplyFilters = true;

	void threadedFunction();

	// Queue holds data ready to be processed
	struct DataElem {
		ofxRTLSEventArgs data;
		ofEvent<ofxRTLSEventArgs>* dataReadyEvent;
	};
	queue< DataElem* > dataQueue;
	ofMutex mtx;

	// Process a data element
	void _process(RTLSProtocol::TrackableFrame& frame);

	// Dictionary for mapping IDs
	IDDictionary dict;
	string dictPath = "";

	// Filters for smoothing data, etc.
	ofxFilterGroup filters;
	// Filter keys will be:
	// if frame.ID < 0:		key = ofToString(frame.ID)		variable length (< 16 digits)
	// else:				key = frame.cuid				16 characters long
	string getFilterKey(const Trackable& t);
	// When was the last time filters were culled? (ms)
	uint64_t lastFilterCullingTime = 0;
	// What is the period by which filters are culled? (ms)
	uint64_t filterCullingPeriod = 1000; // each second

};

#endif