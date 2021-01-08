#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "ofxRTLSTypes.h"
#include "Trackable.pb.h"
#include "ofxRTLSConfigManager.h"
using namespace RTLSProtocol;

#include "ofxRTLSNullSystem.h"
#include "ofxOpenVRTracker.h"
#include "ofxMotive.h"

#include "ofxRTLSPostprocessor.h"

#include "ofxRTLSRecorder.h"
#include "ofxRTLSPlayer.h"

class ofxRTLS : public ofThread {
public:

	// Create an object to connect with the tracking system.
	ofxRTLS();
	~ofxRTLS();

	// Setup this addon.
	void setup();

	// Start the tracking system.
	void start();

	// Stop the tracking system.
	void stop();

	void exit();

	int isConnected();
	bool isReceivingData();
	float getFPS() { return dataFPS; }
	float getMaxSystemFPS();
	double getLatencyMS() { return latencyMS; }

	// Event that occurs when new data is received
	ofEvent< ofxRTLSEventArgs > newFrameReceived;

	// Event that occurs when new latency is calculated
	ofEvent< ofxRTLSLatencyArgs > latencyCalculated;

	// What systems does this version of RTLS support?
	string getSupport();
	string getSupportedSystems();
	bool isPostprocessSupported();
	bool isPlayerSupported();
	bool isRecording();
	void toggleRecording();
	void toggleRecordingWithSavePrompt();
	float getRecordingDuration();
	bool isSavingRecording();
	float getSavingRecordingPercentComplete();
	bool isPlaying();
	bool isPlaying(RTLSSystemType systemType);
	void togglePlayback();
	void resetPlayback();
	void promptOpenPlaybackFile();
	float getPlayingPercentComplete();
	float getPlayingFileDuration();
	string getRecordingFile();
	string getPlayingFile();

private:

	ofxRTLSNullSystem nsys;
	void nsysDataReceived(NullSystemEventArgs& args);
	uint64_t nsysFrameID = 0;
	ofxRTLSPostprocessor nsysPostM;

	ofxOpenVRTracker openvr;
	void openvrDataReceived(ofxOpenVRTrackerEventArgs& args);
	uint64_t openvrFrameID = 0; 
	ofxRTLSPostprocessor openvrPostM; // marker postprocessor

	ofxMotive motive;
	void motiveDataReceived(MotiveEventArgs& args);
	uint64_t motiveFrameID = 0; // increment for every packet sent
	ofxRTLSPostprocessor motivePostM;	// marker postprocessor
	ofxRTLSPostprocessor motivePostR;	// reference (camera) postprocessor
	bool bSendCameraData = true;
	float cameraDataFrequency = 10.0; // what is the sending period in seconds?
	uint64_t lastSendTime = 0;

	ofxRTLSRecorder recorder;

	ofxRTLSPlayer player;
	void playerDataReceived(ofxRTLSPlayerDataArgs& args);
	// frame ID?

	void threadedFunction();

	// last time a packet of data was received
	atomic<bool> bReceivingData = false;
	uint64_t lastReceive = 0;
	int stopGap = 100; // number of milliseconds before we decide no data is being received
	// Mark that we received a new frame
	void markDataReceived();
	// Send event args given a system and type.
	// If the provided system is not compiled, will return false.
	bool sendData(ofxRTLSEventArgs& args);

	// Contains timestamps at which data was received in the last second
	queue<uint64_t> dataTimestamps;
	double dataFPS = 0.0;

	atomic<double> latencyMS = 0.0;
	void newLatencyCalculated(ofxRTLSLatencyArgs& args);
};