#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxOsc.h"

#ifdef RTLS_VIVE 
#include "ofxOpenVRTracker.h"
#endif
#ifdef RTLS_MOTIVE
#include "ofxMotive.h"
#endif

class ofxRTLS {
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


private:

	// OSC Sender
	ofxOscSender sender;
	string oscHost = "127.0.0.1";
	int oscPort = 8282;
	string messageAddress = "/rtls";

	//string positionOrder = "xyz";
	//string orientationOrder = "wxyz";
	//string order = "ipo";

	//bool bForceSendID = false;
	//bool bForceSendPosition = false;
	//bool bForceSendOrientation = false;

#ifdef RTLS_VIVE
	ofxOpenVRTracker vive;
#endif
#ifdef RTLS_MOTIVE
	ofxMotive motive;
	void motiveDataReceived(MotiveEventArgs& args);
#endif


};