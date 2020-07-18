#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

#ifdef RTLS_PLAYER

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
class ofxRTLSRecorder : public ofThread {
public:

	ofxRTLSRecorder();
	~ofxRTLSRecorder();

	void setup(string _takeFolder = "", string _takePrefix = "");

	void update(RTLSProtocol::TrackableFrame& _frame);
	//void update(vector<RTLSProtocol::TrackableFrame*> _frames);

	string getStatus();

	bool isRecording() { return bRecording; }

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
		queue<RTLSProtocol::TrackableFrame*> data;
		string takePath = "";
		uint64_t startTimeMS = 0;
		bool bFlagSave = false;
		void clear() {
			while (!data.empty()) {
				delete data.front();
				data.pop();
			}
		}
	};
	queue< RTLSTake* > takeQueue;
};

#endif