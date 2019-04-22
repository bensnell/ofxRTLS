#include "ofxRTLS.h"

// --------------------------------------------------------------
ofxRTLS::ofxRTLS() {

}

// --------------------------------------------------------------
ofxRTLS::~ofxRTLS() {

}

// --------------------------------------------------------------
void ofxRTLS::setupParams() {

	RUI_NEW_GROUP("ofxRTLS");
	RUI_SHARE_PARAM_WCN("OSC Host", oscHost);
	RUI_SHARE_PARAM_WCN("OSC Port", oscPort, 0, 9999);
	RUI_SHARE_PARAM_WCN("Message Address", messageAddress);

	// Setup RUI Params
#ifdef RTLS_VIVE
	
#endif
#ifdef RTLS_MOTIVE
	motive.setupParams();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::setup() {

	// Setup OSC
	sender.setup(oscHost, oscPort);

	// Add listeners for new data
#ifdef RTLS_VIVE

#endif
#ifdef RTLS_MOTIVE
	ofAddListener(motive.newDataReceived, this, ofxRTLS::motiveDataReceived);
#endif

}

// --------------------------------------------------------------
void ofxRTLS::start() {
	// Start communications with the trackers
#ifdef RTLS_VIVE

#endif
#ifdef RTLS_MOTIVE
	motive.start();
#endif

}

// --------------------------------------------------------------
void ofxRTLS::stop() {
	// Stop communication with the trackers
#ifdef RTLS_VIVE

#endif
#ifdef RTLS_MOTIVE
	motive.stop();
#endif
}

// --------------------------------------------------------------
#ifdef RTLS_MOTIVE

void ofxRTLS::motiveDataReceived(MotiveEventArgs& args) {

	// Send each identified point
	for (int i = 0; i < args.markers.size(); i++) {

		ofxOscMessage m;
		m.setAddress(messageAddress);

		m.addIntArg(args.markers[i].ID);
		m.addFloatArg(args.markers[i].position.x);
		m.addFloatArg(args.markers[i].position.y);
		m.addFloatArg(args.markers[i].position.z);
		
		// should this also send whether it's new data or old data? (keep alive)
		// should this send time?
		
		sender.sendMessage(m, false);
	}
}

#endif
// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------