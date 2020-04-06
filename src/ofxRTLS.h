#pragma once

#if !defined(RTLS_OPENVR) && !defined(RTLS_MOTIVE)
#error "ofxRTLS: Please add one of the following definitions to your project RTLS_OPENVR, RTLS_MOTIVE"
#endif


#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "Trackable.pb.h"
#include "ofxFDeep.h"
#include "ofxFilter.h"

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

	/// \brief Setup the RUI Params associated with this addon
	void setupParams();

	/// \brief Setup this addon
	void setup();

	/// \brief Begin streaming and reconstructing information from the cameras
	void start();

	/// \brief Stop streaming and reconstructing
	void stop();

	void exit();

	bool isConnected();
	bool isReceivingData();

	// Event that occurs when new data is received
	ofEvent< RTLSEventArgs > newFrameReceived;


#ifdef RTLS_OPENVR
	ofxOpenVRTracker vive;
	void openvrDataReceived(ofxOpenVRTrackerEventArgs& args);
#endif
#ifdef RTLS_MOTIVE
	ofxMotive motive;
	void motiveDataReceived(MotiveEventArgs& args);
#endif

private:

	TrackableFrame lastFrame;

	void threadedFunction();

	// last time a packet of data was received
	bool bReceivingData = false;
	uint64_t lastReceive = 0;
	int stopGap = 100; // number of milliseconds before we decide no data is being received
};