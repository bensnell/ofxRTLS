#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){

	ofSetVerticalSync(true);
	ofSetFrameRate(120);

	RUI_SETUP();
	tracker.setupParams();
	RUI_LOAD_FROM_XML();
	
	tracker.setOscEnabled(false);
	tracker.setup();
	tracker.start();
}

//--------------------------------------------------------------
void ofApp::update(){

	tracker.vive.devices.getTrackers();
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(200);

	ofSetColor(0);
	tracker.drawStatus(10, 20);

}

void ofApp::exit() {

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
