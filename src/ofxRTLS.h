#pragma once

#if !defined(RTLS_OPENVR) && !defined(RTLS_MOTIVE)
#error "ofxRTLS: Please add one of the following definitions to your project RTLS_OPENVR, RTLS_MOTIVE"
#endif


#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "Trackable.pb.h"

#ifdef RTLS_ENABLE_POSTPROCESS
#include "IDDictionary.h"
#include "ofxFDeep.h"
#include "ofxFilterGroup.h"
#endif

using namespace RTLSProtocol;

#ifdef RTLS_OPENVR
#include "ofxOpenVRTracker.h"
#endif
#ifdef RTLS_MOTIVE
#include "ofxMotive.h"
#endif

// The event args output
class RTLSEventArgs : public ofEventArgs {
public:
	TrackableFrame frame;
};

class ofxRTLS : public ofThread {
public:

	/// \brief Create an object to connect with motive's cameras. There should only be one per program.
	ofxRTLS();
	~ofxRTLS();

	/// \brief Setup this addon
	void setup();

	/// \brief Begin streaming and reconstructing information from the cameras
	void start();

	/// \brief Stop streaming and reconstructing
	void stop();

	void exit();

	bool isConnected();
	bool isReceivingData();
	float getFPS() { return dataFPS; }

	// Event that occurs when new data is received
	ofEvent< RTLSEventArgs > newFrameReceived;


#ifdef RTLS_OPENVR
	ofxOpenVRTracker vive;
	void openvrDataReceived(ofxOpenVRTrackerEventArgs& args);
#endif
#ifdef RTLS_MOTIVE
	ofxMotive motive;
	void motiveDataReceived(MotiveEventArgs& args);

	bool bSendCameraData = true;
	float cameraDataFrequency = 10.0; // what is the sending period in seconds?
	uint64_t lastSendTime = 0;
#endif

private:

	void threadedFunction();

	// last time a packet of data was received
	bool bReceivingData = false;
	uint64_t lastReceive = 0;
	int stopGap = 100; // number of milliseconds before we decide no data is being received

	// Contains timestamps at which data was received in the last second
	queue<uint64_t> dataTimestamps;
	float dataFPS = 0.0;
	ofMutex mtx;

	// Post-process the data
	void postprocess(RTLSProtocol::TrackableFrame& frame);
#ifdef RTLS_ENABLE_POSTPROCESS
	// Toggles
	bool bMapIDs = true;
	bool bRemoveInvalidIDs = true;
	bool bApplyFilters = true;
	// Post-processing helpers
	IDDictionary dict;
	ofxFilterGroup filters;
	// Filter keys will be:
	// if frame.ID < 0:		key = ofToString(frame.ID)		variable length (< 16 digits)
	// else:				key = frame.cuid				16 characters long
	string getFilterKey(const Trackable& t);
#endif
	
	// Frame ID
	// Increment for each new packet of data sent.
	uint64_t frameID = 0;

};