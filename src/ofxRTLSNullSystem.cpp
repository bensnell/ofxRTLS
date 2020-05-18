#include "ofxRTLSNullSystem.h"

// --------------------------------------------------------------
ofxRTLSNullSystem::ofxRTLSNullSystem() {

}

// --------------------------------------------------------------
ofxRTLSNullSystem::~ofxRTLSNullSystem() {
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::setup() {

	RUI_NEW_GROUP("ofxRTLS Null System");
	RUI_SHARE_PARAM_WCN("NuS- Send Fake Data", bSendFakeData);
	RUI_SHARE_PARAM_WCN("NuS- Frame Rate", fps, 0, 1000);
	RUI_SHARE_PARAM_WCN("NuS- nPoints", nPoints, 0, 1000);
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::start() {
	startThread();
	bConnected = true;
}

// --------------------------------------------------------------
bool ofxRTLSNullSystem::isConnected() {
	return bConnected;
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::stop() {
	bConnected = false;
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::threadedFunction() {

	while (isThreadRunning()) {

		uint64_t thisTime = ofGetElapsedTimeMillis();
		if (float(thisTime - lastTime) >= (1000.0 / fps)) {
			lastTime = thisTime;

			// Export data
			int lastSize = points.size();
			int thisSize = nPoints;
			if (lastSize != thisSize) {
				points.resize(thisSize);
				for (int i = lastSize; i < thisSize; i++) {
					points[i] = glm::vec3(ofRandom(1), ofRandom(1), ofRandom(1));
				}
			}

			// Update all points
			for (int i = 0; i < points.size(); i++) {
				points[i] += glm::vec3(ofRandom(-0.1, 0.1), ofRandom(-0.1, 0.1), ofRandom(-0.1, 0.1));

			}

			NullSystemEventArgs args;
			args.markers = points;
			ofNotifyEvent(newDataReceived, args);

			// Sleep for a bit
			sleep(int(floor(1000.0 / fps * 0.99)));
		}
		else {
			sleep(1);
		}
	}
}

// --------------------------------------------------------------

// --------------------------------------------------------------
