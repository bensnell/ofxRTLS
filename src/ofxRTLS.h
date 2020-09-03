#pragma once

//#if !defined(RTLS_OPENVR) && !defined(RTLS_MOTIVE)
//#error "ofxRTLS: Please add one of the following definitions to your project RTLS_OPENVR, RTLS_MOTIVE"
//#endif

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

#ifdef RTLS_NULL
#include "ofxRTLSNullSystem.h"
#endif
#ifdef RTLS_OPENVR
#include "ofxOpenVRTracker.h"
#endif
#ifdef RTLS_MOTIVE
#include "ofxMotive.h"
#endif

#ifdef RTLS_POSTPROCESS
#include "ofxRTLSPostprocessor.h"
#endif

#ifdef RTLS_PLAYER
#include "ofxRTLSRecorder.h"
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

	int isConnected();
	bool isReceivingData();
	float getFPS() { return dataFPS; }
	float getMaxSystemFPS();
	double getLatencyMS() { return latencyMS; }

	// Event that occurs when new data is received
	ofEvent< ofxRTLSEventArgs > newFrameReceived;

	// Event that occurs when new latency is calculated
	ofEvent< ofxRTLSLatencyArgs > latencyCalculated;

	// What systems does this version of RTLS support?
	string getSupport();
	string getSupportedSystems();
	bool isPostprocessSupported();
	bool isPlayerSupported();

#ifdef RTLS_NULL
	ofxRTLSNullSystem nsys;
	void nsysDataReceived(NullSystemEventArgs& args);
	uint64_t nsysFrameID = 0;

#ifdef RTLS_POSTPROCESS
	ofxRTLSPostprocessor nsysPostM;
#endif
#endif

#ifdef RTLS_OPENVR
	ofxOpenVRTracker openvr;
	void openvrDataReceived(ofxOpenVRTrackerEventArgs& args);
	uint64_t openvrFrameID = 0; 

#ifdef RTLS_POSTPROCESS
	ofxRTLSPostprocessor openvrPostM; // marker postprocessor
#endif
#endif

#ifdef RTLS_MOTIVE
	ofxMotive motive;
	void motiveDataReceived(MotiveEventArgs& args);
	uint64_t motiveFrameID = 0; // increment for every packet sent

#ifdef RTLS_POSTPROCESS
	ofxRTLSPostprocessor motivePostM;	// marker postprocessor
	ofxRTLSPostprocessor motivePostR;	// reference (camera) postprocessor
#endif

	bool bSendCameraData = true;
	float cameraDataFrequency = 10.0; // what is the sending period in seconds?
	uint64_t lastSendTime = 0;
#endif

#ifdef RTLS_PLAYER
	ofxRTLSRecorder recorder;
#endif

	enum RTLS_SYSTEM_TYPE {
		RTLS_SYSTEM_TYPE_NULL = 0,
		RTLS_SYSTEM_TYPE_OPENVR,
		RTLS_SYSTEM_TYPE_MOTIVE,
		NUM_RTLS_SYSTEM_TYPES
	};
	enum RTLS_TRACKABLE_TYPE {
		RTLS_TRACKABLE_TYPE_SAMPLE = 0, // marker
		RTLS_TRACKABLE_TYPE_OBSERVER, // camera, reference
		NUM_RTLS_TRACKABLE_TYPES
	};

private:

	void threadedFunction();

	// last time a packet of data was received
	atomic<bool> bReceivingData = false;
	uint64_t lastReceive = 0;
	int stopGap = 100; // number of milliseconds before we decide no data is being received

	// Contains timestamps at which data was received in the last second
	queue<uint64_t> dataTimestamps;
	double dataFPS = 0.0;

	atomic<double> latencyMS = 0.0;
	void newLatencyCalculated(ofxRTLSLatencyArgs& args);
};