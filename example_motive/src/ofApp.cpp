#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(120);
	RUI_SHARE_PARAM_WCN("Enable Osc", bOscEnabled);
	RUI_SHARE_PARAM_WCN("OSC Host", oscHost);
	RUI_SHARE_PARAM_WCN("OSC Port", oscPort, 0, 9999);
	RUI_SHARE_PARAM_WCN("Message Address", messageAddress);

	RUI_SETUP();
	tracker.setupParams();
	RUI_LOAD_FROM_XML();

	// Setup OSC
	sender.setup(oscHost, oscPort);
	
	tracker.setup();

	tracker.start();

	ofAddListener(tracker.newFrameReceived, this, &ofApp::RTLSFrameReceived);
}

//--------------------------------------------------------------
void ofApp::update(){


}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(200);

	ofSetColor(0);
	drawStatus(10, 20);

}

void ofApp::exit() {

	tracker.exit();

}

void ofApp::RTLSFrameReceived(RTLSEventArgs& args) {

	if (!bOscEnabled) return;

	// Send each identified point
	ofxOscMessage m;
	for (int i = 0; i < args.frame.trackables_size(); i++) {

		m.clear();
		m.setAddress(messageAddress);

		//m.addInt64Arg(args.frame.trackables(i).cuid());
		m.addFloatArg(args.frame.trackables(i).position().x());
		m.addFloatArg(args.frame.trackables(i).position().y());
		m.addFloatArg(args.frame.trackables(i).position().z());

		// should this also send whether it's new data or old data? (keep alive)
		// should this send time?

		sender.sendMessage(m, false);

		cout << m << endl;

		// Log this data
		lastSend = ofGetElapsedTimeMillis();
	}

	// Save this message
	lastMessage = m;
}

// --------------------------------------------------------------
void ofApp::drawStatus(int x, int y) {

	stringstream ss;
	ss << "Sending OSC over " << oscHost << " : " << ofToString(oscPort) << "\n";
	ss << "Message address: \"" << messageAddress << "\"\n";
	ss << "Connected?\t" << (tracker.isConnected() ? "TRUE" : "FALSE") << "\n";
	ss << "Sending?\t" << (bSending ? "TRUE" : "FALSE") << "\n";
	//if (bSending) { // throws errors
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
//void ofApp::threadedFunction() {
//
//	while (isThreadRunning()) {
//
//		// Determine whether we are actively sending any messages in the last
//		// stopGap milliseconds
//		bSending = ofGetElapsedTimeMillis() - lastSend < stopGap;
//
//	}
//}

// --------------------------------------------------------------
void ofApp::setOscEnabled(bool _bOscEnabled) {
	bOscEnabled = _bOscEnabled;
}

// --------------------------------------------------------------
bool ofApp::isOscSending() {
	return bSending;
}

// --------------------------------------------------------------
string ofApp::getOscHostAddress() {
	return oscHost;
}

// --------------------------------------------------------------
int ofApp::getOscPort() {
	return oscPort;
}

// --------------------------------------------------------------
string ofApp::getOscMessageAddress() {
	return messageAddress;
}

// --------------------------------------------------------------
bool ofApp::isOscEnabled() {
	return bOscEnabled;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
