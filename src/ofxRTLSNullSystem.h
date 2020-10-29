#pragma once

#include "ofMain.h"

#include "ofxRemoteUIServer.h"
#include "ofxTemporalResampler.h"

class NullSystemTrackable {
public:
	NullSystemTrackable() {};
	~NullSystemTrackable() {};

	bool hasPosition() { return bPosition; }
	glm::vec3 getPosition() { return position + noise; }
	glm::vec3 getKnownPosition() { return position; }
	void setKnownPosition(glm::vec3 _position) { position = _position; bPosition = true; }
	void setPositionNoise(glm::vec3 _noise) { noise = _noise; }
	void clearPosition() { bPosition = false; }

	bool hasId() { return bId; }
	int getId() { return id; }
	void setId(int _id) { id = _id; bId = true; }
	void clearId() { bId = false; }

	bool hasCuid() { return bCuid; }
	string getCuid() { return cuid; }
	void setCuid(string _cuid) { cuid = _cuid; bCuid = true; }
	void clearCuid() { bCuid = false; }

	glm::vec3 force;	// forces
	glm::vec3 velocity;	// velocity
	float maxSpeedFraction = 1.0;

private:
	bool bPosition = false;
	glm::vec3 position;
	glm::vec3 noise;

	bool bId = false;
	int id;

	bool bCuid = false;
	string cuid;
};

class _NullSystemTrackable : public NullSystemTrackable {
public:
	bool bPresent = true;
};

// The event args output
class NullSystemEventArgs : public ofEventArgs {
public:
	vector<NullSystemTrackable> trackables;
	bool bOverrideContext = false;
	int systemOverride = 0;
	int typeOverride = 0;
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

	float getFrameRate() { return fps; }

private:

	bool bConnected = false;

	void threadedFunction();

	// Params
	bool bSendFakeData = false;
	// Mode of the fake data
	enum RTLSNullSystemMode {
		ELASTIC = 0,
		BROWNIAN,
		NUM_MODES
	};
	RTLSNullSystemMode mode = ELASTIC;
	float elasticForce = 1.0; // [0, +inf)
	float elasticWinds = 0.01; // amount [0, 1]
	float fps = 30;		// framerate at which to send data
	int nPoints = 1;	// numbers of points to send
	// Bounds of the space that constrains the trackables.
	float loBound = 0;	// in x,y,z
	float hiBound = 1;	// in x,y,z
	// Speed of the trackables.
	float positionSpeed = 0.5;
	// Amount of spatial noise in output position.
	// (in spatial units; standard deviation of gaussian normal)
	float positionNoise = 0.001;
	// What is the target presence density?
	// In other words, what fraction of the time should the trackable be present?
	// Range: [0, 1]
	float targetPresenceDensity = 0.95;
	// How quickly should the trackable return to being present after becoming absent?
	// Range: [0, +inf)
	// This value is related to time. 0 indicates that it returns as fast as possible,
	// usually the next frame. 1 indicates that it will on average take a full second
	// to return. 2 indicates that it will take about 2 seconds to return, etc.
	float presenceReturnRapidness = 1.0;
	// Should ID be exported?
	bool bSetID = true;
	// Should CUID be exported?
	// CUID will be new after every absence.
	bool bSetCUID = false;

	// Force a different system and/or type
	bool bOverrideContext = false;
	int systemOverride = 0;
	int typeOverride = 0;

	void getNextFrame(NullSystemEventArgs& args);
	vector<_NullSystemTrackable> trackables;

	uint64_t cuidCounter = 1;

	// dynamically sets the output frame rate
	void paramChanged(RemoteUIServerCallBackArg& arg);
	ofxTemporalResampler temporalResampler;
};