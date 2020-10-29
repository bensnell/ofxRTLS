#pragma once

#include "ofMain.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;
#include "ofxRTLSTypes.h"

class ofxRTLSLatencyArgs : public ofEventArgs {
public:
	uint64_t startTimeUS = 0;
	uint64_t stopTimeUS = 0;
	bool isValid() { return startTimeUS != 0 && stopTimeUS != 0; }
};

// The event args output
class ofxRTLSEventArgs : public ofEventArgs {
public:
	
	ofxRTLSEventArgs() {};
	ofxRTLSEventArgs(ofEvent<ofxRTLSLatencyArgs>& _newLatencyCalculated) {
		newLatencyCalculated = &_newLatencyCalculated;
		latency.startTimeUS = ofGetElapsedTimeMicros();
	}
	~ofxRTLSEventArgs() {
		latency.stopTimeUS = ofGetElapsedTimeMicros();
		// Send if not already sent
		if (!bSent) sendLatency();
	}

	// Data that will be sent
	TrackableFrame frame;

	// (Optional, for more accuracy)
	// Set the time this data was first being assembled.
	void flagStartAssembly() {
		latency.startTimeUS = ofGetElapsedTimeMicros();
	}
	void setStartAssemblyTime(uint64_t _timeMicros) {
		latency.startTimeUS = _timeMicros;
	}

	// (Optional, for more accuracy)
	// Flag that this frame has been sent.
	void flagSent() {
		latency.stopTimeUS = ofGetElapsedTimeMicros();
		sendLatency();
	}

	// Purposely nullify this data
	void nullify() { 
		bValid = false;
	}

	void copyFrom(const ofxRTLSEventArgs& other) {
		frame = other.frame;
		newLatencyCalculated = other.newLatencyCalculated;
		latency = other.latency;
		bSent = other.bSent;
		bValid = other.bValid;
	}

private:
	ofEvent<ofxRTLSLatencyArgs>* newLatencyCalculated;
	ofxRTLSLatencyArgs latency;
	bool bSent = false;
	bool bValid = true;
	void sendLatency() {
		if (!bValid) return;
		if (newLatencyCalculated == NULL) return;
		if (!latency.isValid()) return;
		bSent = true;
		ofNotifyEvent(*newLatencyCalculated, latency);
	}
};

// Argument when a recording is complete
class ofxRTLSRecordingArgs : public ofEventArgs {
public:
	bool bRecordingBegan = false;
	bool bRecordingEnded = false;
	string filePath = "";
};

class ofxRTLSPlayerLoopedArgs : public ofEventArgs {
public:
	// All system and type pairs that have just looped
	vector< pair<RTLSSystemType, RTLSTrackableType> > systems;
};

// Argument when a file begins or stops playing
class ofxRTLSPlaybackArgs : public ofEventArgs {
public:
	bool bPlay = false;
	bool bPause = false;
};