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
class ofxRTLSRecorder : public ofThread {
public:

	ofxRTLSRecorder();
	~ofxRTLSRecorder();

	void setup(string _takeFolder = "", string _takePrefix = "");

	void update(RTLSProtocol::TrackableFrame& _frame);

	string getStatus();

	// not safe
	bool isRecording() { return bRecord; }

private:

	bool bEnableRecorder = true;
	bool bRecord = false;
	bool isSetup = false;

	bool bRecording = false;
	string takeFolder = "takes";
	string takePrefix = "take"; // name will be takePrefix + "_" + timestamp + ".c3d"

	bool isRecordingPrecise() { return !takeQueue.empty() && takeQueue.back() != NULL; }

	void paramChanged(RemoteUIServerCallBackArg& arg);

	void threadedFunction();
	std::condition_variable cv;
	atomic<bool> flagUnlock = false;

	// Queue holds data ready to be saved
	struct RTLSTake {
		queue<RTLSProtocol::TrackableFrame*> data;
		string takePath = "";
		uint64_t startTimeMS = 0;
	};
	queue< RTLSTake* > takeQueue;

};

#endif