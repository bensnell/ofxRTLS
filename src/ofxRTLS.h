#pragma once

#if !defined(RTLS_OPENVR) && !defined(RTLS_MOTIVE)
#error "ofxRTLS: Please add one of the following definitions to your project RTLS_OPENVR, RTLS_MOTIVE"
#endif

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

#ifdef RTLS_OPENVR
#include "ofxOpenVRTracker.h"
#endif
#ifdef RTLS_MOTIVE
#include "ofxMotive.h"
#endif

#ifdef RTLS_ENABLE_POSTPROCESS
#include "ofxRTLSPostprocessor.h"
#endif

class ofxRTLS : public ofThread {
public:

	// Create an object to connect with the tracking system.
	ofxRTLS();
	~ofxRTLS();

	// Setup this addon.
	void setup();

	// Start the tracking system.
	void start();

	// Stop the tracking system.
	void stop();

	void exit();

	bool isConnected();
	bool isReceivingData();
	float getFPS() { return dataFPS; }

	// Event that occurs when new data is received
	ofEvent< ofxRTLSEventArgs > newFrameReceived;

#ifdef RTLS_OPENVR
	ofxOpenVRTracker vive;
	void openvrDataReceived(ofxOpenVRTrackerEventArgs& args);

#ifdef RTLS_ENABLE_POSTPROCESS
	ofxRTLSPostprocessor tPost; // tracker postprocessor
#endif
#endif

#ifdef RTLS_MOTIVE
	ofxMotive motive;
	void motiveDataReceived(MotiveEventArgs& args);

#ifdef RTLS_ENABLE_POSTPROCESS
	ofxRTLSPostprocessor mPost;	// marker postprocessor
	ofxRTLSPostprocessor cPost;	// camera postprocessor
#endif

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
	
	// Frame ID
	// Increment for each new packet of data sent.
	uint64_t frameID = 0;

};