#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

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


class ofxRTLSRecorder : public ofThread {
public:

	ofxRTLSRecorder();
	~ofxRTLSRecorder();

	void setup(string _takeFolder = "", string _takePrefix = "");

	void update(RTLSProtocol::TrackableFrame& _frame);

	string getStatus();

private:

	bool bEnableRecorder = true;
	bool bFlagStart = false;
	bool bFlagStop = false;

	bool bRecording = false;
	string takeFolder = "takes";
	string takePrefix = "take"; // name will be takePrefix + "_" + timestamp + ".c3d"


	string thisTakePath = "";
	uint64_t thisTakeStartTimeMS = 0;

	void paramChanged(RemoteUIServerCallBackArg& arg);

	void threadedFunction();
	std::condition_variable cv;
	atomic<bool> flagUnlock = false;

	// Queue holds data ready to be saved
	queue< RTLSProtocol::TrackableFrame* > dataQueue;

};
