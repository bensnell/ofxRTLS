#pragma once

#include "ofMain.h"

#include "ofxRemoteUIServer.h"

// The event args output
class NullSystemEventArgs : public ofEventArgs {
public:
	vector<glm::vec3> markers;
};

class ofxRTLSNullSystem : private ofThread {
public:

	ofxRTLSNullSystem();
	~ofxRTLSNullSystem();

	void setup();

	void start();

	bool isConnected();

	void stop();

	ofEvent< NullSystemEventArgs > newDataReceived;

private:

	bool bConnected = false;

	void threadedFunction();

	// Params
	bool bSendFakeData = false;
	float fps = 30;		// framerate at which to send data
	int nPoints = 1;	// numbers of points to send
	
	vector<glm::vec3> points;
	uint64_t lastTime = 0; // ms

};