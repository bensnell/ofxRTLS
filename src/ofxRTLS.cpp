#include "ofxRTLS.h"

// --------------------------------------------------------------
ofxRTLS::ofxRTLS() {

}

// --------------------------------------------------------------
ofxRTLS::~ofxRTLS() {
	stopThread();
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
	motive.setCalibrationPath(ofToDataPath("6c_20190418.cal"));
	motive.setProfilePath(ofToDataPath("6c_20190418.motive"));
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
	ofAddListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif

	startThread();
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
	ofxOscMessage m;
	for (int i = 0; i < args.markers.size(); i++) {

		m.clear();
		m.setAddress(messageAddress);

		m.addIntArg(args.markers[i].ID);
		m.addFloatArg(args.markers[i].position.x);
		m.addFloatArg(args.markers[i].position.y);
		m.addFloatArg(args.markers[i].position.z);
		
		// should this also send whether it's new data or old data? (keep alive)
		// should this send time?
		
		sender.sendMessage(m, false);
	}

	// Log this data
	lastSend = ofGetElapsedTimeMillis();
	// Save this message
	lastMessage = m;
}

#endif
// --------------------------------------------------------------
void ofxRTLS::drawStatus(int x, int y) {

	stringstream ss;
	ss << "Sending OSC over " << oscHost << " : " << ofToString(oscPort) << "\n";
	ss << "Message address: \"" << messageAddress << "\"\n";
	ss << "Sending?\t" << (isSending ? "TRUE" : "FALSE") << "\n";
	//if (isSending) { // throws errors
	//	ofxOscMessage tmp = lastMessage;
	//	ss << "Last Message:\n";
	//	for (int i = 0; i < tmp.getNumArgs(); i++) {
	//		ss << "\t" << tmp.getArgAsString(i);
	//	}
	//	ss << "\n";
	//}
	ofDrawBitmapString(ss.str(), x, y);
}

// --------------------------------------------------------------
void ofxRTLS::threadedFunction() {

	while (isThreadRunning()) {

		// Determine whether we are actively sending any messages in the last
		// stopGap milliseconds
		isSending = ofGetElapsedTimeMillis() - lastSend < stopGap;

	}
}

// --------------------------------------------------------------
void ofxRTLS::exit() {

#ifdef RTLS_MOTIVE
	ofRemoveListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif

}

// --------------------------------------------------------------

// --------------------------------------------------------------