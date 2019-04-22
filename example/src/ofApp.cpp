#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(120);

	RUI_SETUP();
	motive.setupParams();
	RUI_LOAD_FROM_XML();
	

	motive.setCalibrationPath(ofToDataPath("6c_20190418.cal"));
	motive.setProfilePath(ofToDataPath("6c_20190418.motive"));
	motive.start();
}

//--------------------------------------------------------------
void ofApp::update(){

	vector<MotiveOutput> out = motive.get3DPoints();
	stringstream ss;
	ss << "Motive Output\n";
	for (int i = 0; i < out.size(); i++) {
		ss << "\t" << out[i].position << "\n";
	}
	if (out.size() > 0) cout << ss.str() << endl;

}

//--------------------------------------------------------------
void ofApp::draw(){

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
