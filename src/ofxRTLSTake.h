#pragma once

#include "ofMain.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;
#include "ofxRTLSTrackableKey.h"
#include "ofxRTLSTypes.h"

#ifdef RTLS_PLAYER

#include "ezc3d_all.h"

// Frame data from a single system
struct RTLSTakeSystemData {
	queue< vector< RTLSProtocol::TrackableFrame* > > frames;
	vector< RTLSProtocol::TrackableFrame* > nextFrame;
	void addNextFrame() {
		frames.push(nextFrame);
		nextFrame.clear();
	}
	void clearAndPopNextFrame() {
		if (frames.empty()) return;
		for (int i = 0; i < frames.front().size(); i++) {
			frames.front()[i]->Clear();
			delete frames.front()[i];
		}
		frames.front().clear();
		frames.pop();
	}
	void clear() {
		for (auto f : nextFrame) {
			f->Clear();
			delete f;
		}
		nextFrame.clear();
		while (!frames.empty()) {
			clearAndPopNextFrame();
		}
	}
	int size() { return frames.size(); }
	bool empty() { return frames.empty(); }
};

// RTLSTake contains a single take for recording or playing.
// (Queue holds data that is actively being written to or saved.)
class RTLSTake {
public:

	RTLSTake() {};
	~RTLSTake() {};

	// All pieces of data present in this take
	// Map of system index to a queue of frames
	map<int, RTLSTakeSystemData > data;
	bool systemExists(int systemIndex) {
		return data.find(systemIndex) != data.end();
	}
	// Start time of this take
	uint64_t startTimeMS = 0;
	// Flag whether we want to save this take
	bool bFlagSave = false;
	// What is the global FPS for this take?
	float fps = -1;

	// Is this take empty?
	bool empty() {
		if (data.size() == 0) return true;
		for (auto it : data) {
			if (!it.second.empty()) return false;
		}
		return true;
	}

	// Is this take valid?
	bool valid() {
		return fps > 0;
	}

	// Clear all the data in this take
	void clear() {
		auto it = data.begin();
		while (it != data.end()) {
			it->second.clear();
			it = data.erase(it);
		}
	}

	// C3D data structure
	// This cannot be written in real time because all of the trackable (point)
	// labels must be collected and written to the c3d header before storing
	// positions, etc.
	ezc3d::c3d c3d;
	// Path to save this c3d file
	string path = "";
	// What are all of the present labels that describe the points
	set<string> c3dPointLabels;
	// What are the descriptions for each label?
	map<string, string> c3dPointLabels2Desc;
};

// A separate take object specifically
// for the player, since it requires
// c3d to be dynamically allocated and re-allocated.
// TODO: Merge this and RTLSTake
class RTLSPlayerTake {
public:

	RTLSPlayerTake() {
		c3d = NULL;
	};
	~RTLSPlayerTake() {
		clear();
	};

	ezc3d::c3d* c3d;
	string path = "";

	float getC3dFps() {
		return c3d->header().frameRate();
	};
	float getC3dDurationSec() {
		return float(c3d->header().nbFrames()) /
			c3d->header().frameRate();
	};
	uint64_t getC3dNumFrames() {
		return c3d->header().nbFrames();
	}
	void clear() {
		if (c3d != NULL) {
			//c3d->~c3d();
			delete c3d;
			c3d = NULL;
		}
	}

	// Frames every frame are kept here
	struct Frame {
		TrackableFrame frame; // template
		TrackableFrame newFrame; // new frame
		// Is this new data?
		// This parameter is currently unused and unnecessary, since frames
		// should be processed whether or not they have data. We must do this since
		// the postprocessor should be called to process() data, even if
		// any isn't presently valid.
		bool bNewData = false;
		RTLSSystemType systemType = RTLS_SYSTEM_TYPE_INVALID;
		RTLSTrackableType trackableType = RTLS_TRACKABLE_TYPE_INVALID;
		vector<int> dataIndices;
	};
	vector<Frame> frames;
	void flagAllFramesOld() {
		for (auto& f : frames) f.bNewData = false;
	}
	map<int, int> dataIndexToFrameIndex;
	bool populateTemplateFrames();

	// Current frame (time) index
	uint64_t frameCounter = 0;
};

#endif