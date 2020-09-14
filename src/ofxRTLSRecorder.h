#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "ofxRTLSTake.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;
#include "ofxRTLSTrackableKey.h"

#ifdef RTLS_PLAYER

#include "ezc3d_all.h"

// Locking with Condition Variables, Queues and Mutex follows the 
// examples set forth here:
// https://ncona.com/2019/04/using-condition-variables-in-cpp/
// Also useful:
// https://wiki.sei.cmu.edu/confluence/display/cplusplus/CON54-CPP.+Wrap+functions+that+can+spuriously+wake+up+in+a+loop
// https://en.cppreference.com/w/cpp/thread/condition_variable
// https://www.justsoftwaresolutions.co.uk/threading/multithreading-in-c++0x-part-7-locking-multiple-mutexes.html
// https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables
// http://jakascorner.com/blog/2016/02/lock_guard-and-unique_lock.html

// Note: This recorder does not record *all* information. Instead, it only records what is 
// able to be recoreded by the C3D filetype:
// https://www.c3d.org/

// Records data to the C3D filetype.
// Waits for all data to be added to a recording before writing it to file.
// NOTE: The recorder can save data from different tracking systems,
// but all systems must have the same frame rate.
class ofxRTLSRecorder : public ofThread {
public:

	ofxRTLSRecorder();
	~ofxRTLSRecorder();

	void setup(string _takeFolder = "", string _takePrefix = "");

	// Add any number of TrackableFrames...
	void add(int systemIndex, float systemFPS, RTLSProtocol::TrackableFrame& _frame);
	// ... Then update the frame counter. 
	// Also provide the current framerate. The frame rate at the start of a 
	// recording will be the framerate for the duration of the whole recording.
	void update(int systemIndex);

	string getStatus();

	bool isRecording() { return bRecording; }
	string getRecordingFile() { return thisTakePath; }
	void toggleRecording();

	ofEvent<ofxRTLSRecordingCompleteArgs> recordingComplete;

private:

	bool bEnableRecorder = true;
	bool isSetup = false;

	bool bShouldRecord = false; // signal from user

	atomic<bool> bRecording = false; // are we currently recording?
	string takeFolder = "takes";
	string takePrefix = "take"; // name will be takePrefix + "_" + timestamp + ".c3d"

	string thisTakePath = "";
	uint64_t thisTakeStartTimeMS = 0;

	void paramChanged(RemoteUIServerCallBackArg& arg);

	void threadedFunction();
	std::condition_variable cv;
	atomic<bool> flagUnlock = false;

	// Queue holds data that is actively being written to or saved
	class RTLSTake {
	public:

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
	queue< RTLSTake* > takeQueue;
	
	bool saveTake(RTLSTake* take);
};

#endif