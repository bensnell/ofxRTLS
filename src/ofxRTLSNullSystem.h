#pragma once

#include "ofMain.h"

#include "ofxRemoteUIServer.h"

class NullSystemTrackable {
public:
	NullSystemTrackable() {};
	~NullSystemTrackable() {};

	bool hasPosition() { return bPosition; }
	glm::vec3 getPosition() { return position; }
	void setPosition(glm::vec3 _position) { position = _position; bPosition = true; }
	void clearPosition() { bPosition = false; }

	bool hasId() { return bId; }
	int getId() { return id; }
	void setId(int _id) { id = _id; bId = true; }
	void clearId() { bId = false; }

	bool hasCuid() { return bCuid; }
	string getCuid() { return cuid; }
	void setCuid(string _cuid) { cuid = _cuid; bCuid = true; }
	void clearCuid() { bCuid = false; }

private:
	bool bPosition = false;
	glm::vec3 position;

	bool bId = false;
	int id;

	bool bCuid = false;
	string cuid;
};

class _NullSystemTrackable : public NullSystemTrackable {
public:
	float presence = 1.0;
	queue<bool> presentSamples;
	float presentSamplesSum = 0.0;
	bool bPresent = true;

};

// The event args output
class NullSystemEventArgs : public ofEventArgs {
public:
	vector<NullSystemTrackable> trackables;
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
	// Bounds of the space that constrains the trackables.
	float loBound = 0;	// in x,y,z
	float hiBound = 1;	// in x,y,z
	// Speed of the trackables.
	float positionSpeed = 0.5;
	// Amount of spatial noise in output position.
	// (in spatial units; standard deviation of gaussian normal)
	float positionNoise = 0.01;
	// This threshold describes the target presence in the range [0,1].
	// If 0, then the point will rarely, if ever, be present.
	// If 1, then the point will always be present.
	float presenceThreshold = 1.0;
	// This rapidness describes the quickness to return to threshold.
	// It exists in the range (-inf, +inf). It it correlated to the length
	// of absences.
	// If -1, then absences will be longer and it will take more time
	// to return to target presence (threshold).
	// If +1, then absences will be shorted and it will take less time
	// to return to target presence.
	float presenceRapidness = 0;
	// Should ID be exported?
	bool bSetID = true;
	// Should CUID be exported?
	// CUID will be new after every absence.
	bool bSetCUID = false;

	vector<_NullSystemTrackable> trackables;


	uint64_t lastTime = 0; // ms

	uint64_t cuidCounter = 1;
};