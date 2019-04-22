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

	/// \brief Begin streaming and reconstructing information from the cameras
	void start();

	/// \brief Stop streaming and reconstructing
	void stop();


private:

#ifdef RTLS_VIVE
	ofxOpenVRTracker openvr;
#endif
#ifdef RTLS_MOTIVE
	ofxMotive motive;
#endif

};