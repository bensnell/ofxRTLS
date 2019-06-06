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
	RUI_SHARE_PARAM_WCN("Enable Osc", bOscEnabled);
	RUI_SHARE_PARAM_WCN("OSC Host", oscHost);
	RUI_SHARE_PARAM_WCN("OSC Port", oscPort, 0, 9999);
	RUI_SHARE_PARAM_WCN("Message Address", messageAddress);

	// Setup RUI Params
#ifdef RTLS_VIVE
	// Are there any params?

#endif
#ifdef RTLS_MOTIVE
	motive.setCalibrationPath(ofToDataPath("6c_20190425.cal"));
	motive.setProfilePath(ofToDataPath("6c_20190425.motive"));
	motive.setupParams();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::setup() {

	// Setup OSC
	sender.setup(oscHost, oscPort);

	// Add listeners for new data
#ifdef RTLS_VIVE
	ofAddListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);
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
	vive.connect();
#endif
#ifdef RTLS_MOTIVE
	motive.start();
#endif

}

// --------------------------------------------------------------
void ofxRTLS::stop() {
	// Stop communication with the trackers
#ifdef RTLS_VIVE
	vive.disconnect();
#endif
#ifdef RTLS_MOTIVE
	motive.stop();
#endif
}

// --------------------------------------------------------------
#ifdef RTLS_MOTIVE

void ofxRTLS::motiveDataReceived(MotiveEventArgs& args) {

	if (!bOscEnabled) return;

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

		// Log this data
		lastSend = ofGetElapsedTimeMillis();
	}

	// Save this message
	lastMessage = m;
}

#endif

// --------------------------------------------------------------
#ifdef RTLS_VIVE

void ofxRTLS::openvrDataReceived(ofxOpenVRTrackerEventArgs& args) {

	if (!bOscEnabled) return;

	// Send each identified point
	ofxOscMessage m;

	for (int i = 0; i < (*args.devices->getTrackers()).size(); i++) {

		Device* tkr = (*args.devices->getTrackers())[i];
		if (tkr->isActive()) {

			m.clear();
			m.setAddress(messageAddress);

			m.addStringArg(tkr->serialNumber);
			m.addFloatArg(tkr->position.x);
			m.addFloatArg(tkr->position.y);
			m.addFloatArg(tkr->position.z);
			m.addFloatArg(tkr->quaternion.w);
			m.addFloatArg(tkr->quaternion.x);
			m.addFloatArg(tkr->quaternion.y);
			m.addFloatArg(tkr->quaternion.z);

			// should this also send whether it's new data or old data? (keep alive)
			// should this send time?

			sender.sendMessage(m, false);

			// Log this data
			lastSend = ofGetElapsedTimeMillis();
		}
	}

	// Save this message
	lastMessage = m;
}

#endif

// --------------------------------------------------------------
void ofxRTLS::drawStatus(int x, int y) {

	stringstream ss;
	ss << "Sending OSC over " << oscHost << " : " << ofToString(oscPort) << "\n";
	ss << "Message address: \"" << messageAddress << "\"\n";
	ss << "Connected?\t" << (isConnected() ? "TRUE" : "FALSE") << "\n";
	ss << "Sending?\t" << (isSending ? "TRUE" : "FALSE") << "\n";
	//if (isSending) { // throws errors
	//	ofxOscMessage tmp = lastMessage;
	//	ss << "Last Message:\n";
	//	for (int i = 0; i < tmp.getNumArgs(); i++) {
	//		ss << "\t" << tmp.getArgAsString(i);
	//	}
	//	ss << "\n";
	//}
	ofDrawBitmapStringHighlight(ss.str(), x, y);
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

#ifdef RTLS_VIVE
	// Remove listener for new device data
	ofRemoveListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	vive.exit();
#endif
#ifdef RTLS_MOTIVE
	ofRemoveListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif
}

// --------------------------------------------------------------
bool ofxRTLS::isConnected() {

#ifdef RTLS_VIVE
	return vive.isConnected();
#endif
#ifdef RTLS_MOTIVE
	return motive.isConnected();
#endif

	return false;
}

// --------------------------------------------------------------
void ofxRTLS::setOscEnabled(bool _bOscEnabled) {
	bOscEnabled = _bOscEnabled;
}

// --------------------------------------------------------------


// --------------------------------------------------------------


// --------------------------------------------------------------
