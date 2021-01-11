#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	
	oscThread.setup();
	oscThread.startThread();
}
//--------------------------------------------------------------
void OSCThread::setup(){

	ofSetFrameRate(120);

	RUI_SETUP();
	RUI_LOAD_FROM_XML();

	RUI_SHARE_PARAM_WCN("Enable Osc", bOscEnabled);
	RUI_SHARE_PARAM_WCN("OSC Host", oscHost);
	RUI_SHARE_PARAM_WCN("OSC Port", oscPort, 0, 9999);
	RUI_SHARE_PARAM_WCN("Message Address", messageAddress);

	// Setup OSC
	sender.setup(oscHost, oscPort);
	
	tracker.setup();

	tracker.start();

	ofAddListener(tracker.newFrameReceived, this, &OSCThread::RTLSFrameReceived);
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(200);

	ofSetColor(0);
	oscThread.drawStatus(10, 20);
}

//--------------------------------------------------------------
void ofApp::exit() {

	oscThread.exit();
}

//--------------------------------------------------------------
void OSCThread::RTLSFrameReceived(ofxRTLSEventArgs& args) {

	// cout << args.frame.DebugString() << endl;

	if (!bOscEnabled) return;

	// Send each identified point
	ofxOscMessage m;
	for (int i = 0; i < args.frame.trackables_size(); i++) {

		m.clear();
		m.setAddress(messageAddress);

		// Arg defaults to empty or 0 if not set
		m.addStringArg(args.frame.trackables(i).name());
		m.addInt64Arg(((uint64_t*)args.frame.trackables(i).cuid().c_str())[0]);
		m.addInt64Arg(((uint64_t*)args.frame.trackables(i).cuid().c_str())[1]);
		m.addFloatArg(args.frame.trackables(i).position().x());
		m.addFloatArg(args.frame.trackables(i).position().y());
		m.addFloatArg(args.frame.trackables(i).position().z());
		m.addFloatArg(args.frame.trackables(i).orientation().w());
		m.addFloatArg(args.frame.trackables(i).orientation().x());
		m.addFloatArg(args.frame.trackables(i).orientation().y());
		m.addFloatArg(args.frame.trackables(i).orientation().z());

		//cout << m << endl;

		// Send the message
		sender.sendMessage(m, false);

		// Log this data
		lastSend = ofGetElapsedTimeMillis();
	}

	// Save this message
	lastMessage = m;
}

// --------------------------------------------------------------
void OSCThread::drawStatus(int x, int y) {

	stringstream ss;
	ss << "Sending OSC over " << oscHost << " : " << ofToString(oscPort) << "\n";
	ss << "Message address: \"" << messageAddress << "\"\n";
	ss << "Systems:\t" << "{ " << (RTLS_NULL() ? "NULL " : "") << (RTLS_OPENVR() ? "OPENVR " : "") << (RTLS_MOTIVE() ? "MOTIVE " : "") << "}" << "\n";
	ss << "Connected?\t" << (tracker.isConnected() ? "TRUE" : "FALSE") << "\n";
	ss << "Sending?\t" << (bSending ? "TRUE" : "FALSE") << "\n";
	ss << "Postprocess?\t" << (RTLS_POSTPROCESS() ? "TRUE" : "FALSE") << "\n";
	ss << "Player?\t\t" << (RTLS_PLAYER() ? "TRUE" : "FALSE") << "\n";
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
void OSCThread::threadedFunction() {

	while (isThreadRunning()) {

		// Determine whether we are actively sending any messages in the last
		// stopGap milliseconds
		bSending = ofGetElapsedTimeMillis() - lastSend < stopGap;
	}
}

// --------------------------------------------------------------
void OSCThread::setOscEnabled(bool _bOscEnabled) {
	bOscEnabled = _bOscEnabled;
}

// --------------------------------------------------------------
bool OSCThread::isOscSending() {
	return bSending;
}

// --------------------------------------------------------------
string OSCThread::getOscHostAddress() {
	return oscHost;
}

// --------------------------------------------------------------
int OSCThread::getOscPort() {
	return oscPort;
}

// --------------------------------------------------------------
string OSCThread::getOscMessageAddress() {
	return messageAddress;
}

// --------------------------------------------------------------
bool OSCThread::isOscEnabled() {
	return bOscEnabled;
}

//--------------------------------------------------------------
void OSCThread::exit() {

	stopThread();
	tracker.exit();
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
