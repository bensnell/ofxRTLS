#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "ofxRTLSTypes.h"
#include "ofxRTLSTake.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;
#include "ofxRTLSTrackableKey.h"

#ifdef RTLS_PLAYER

#include "ezc3d_all.h"
#include "ofxTemporalResampler.h"

class ofxRTLSPlayerDataArgs : public ofEventArgs {
public:

	TrackableFrame frame;
	RTLSSystemType systemType = RTLS_SYSTEM_TYPE_INVALID;
	RTLSTrackableType trackableType = RTLS_TRACKABLE_TYPE_INVALID;
};

// Locking with Condition Variables, Queues and Mutex follows the 
// examples set forth here:
// https://ncona.com/2019/04/using-condition-variables-in-cpp/
// Also useful:
// https://wiki.sei.cmu.edu/confluence/display/cplusplus/CON54-CPP.+Wrap+functions+that+can+spuriously+wake+up+in+a+loop
// https://en.cppreference.com/w/cpp/thread/condition_variable
// https://www.justsoftwaresolutions.co.uk/threading/multithreading-in-c++0x-part-7-locking-multiple-mutexes.html
// https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables
// http://jakascorner.com/blog/2016/02/lock_guard-and-unique_lock.html

// Plays data back from a c3d file
// Notes:
// -	On any build, can play back any system. However, 
//		can only be piped through pipelines for which 
//		an app has been built. For example, Motive data can
//		play on an app compiled for Null systems, but
//		will be processed using the Null postprocessing pipeilne.
//		When multiple pipelines exist, user can choose. 
//		Defaults to the same system pipeline data was recorded from.
class ofxRTLSPlayer : public ofThread {
public:

	ofxRTLSPlayer();
	~ofxRTLSPlayer();

	void setup();

	void setPlayingFile(string filePath);
	void promptUserOpenFile();
	string getTakePath() { return takePath; }
	double getTakeDuration() { return durationSec; }
	float getTakeFPS() { return fps; }

	void setLooping(bool _bLoop);
	bool isLooping() { return bLoop; }

	void play();
	void pause();
	void reset(); // to frame 0
	void togglePlayback();
	bool isPlaying() { return bPlaying; }
	bool isPlaying(RTLSSystemType systemType);

	void setOverrideRealtimeData(bool _bOverride);
	bool isOverridingRealtimeData() { return bOverridesRealtimeData; }

	// This is called when a recording has been completed.
	// (Must manually add a listener.)
	void newRecording(ofxRTLSRecordingCompleteArgs& args);

	// This event is called when the player loops
	ofEvent<ofxRTLSPlayerLoopedArgs> takeLooped;

	// This event is notified when new data is available from the player.
	ofEvent<ofxRTLSPlayerDataArgs> newPlaybackData;

	string getStatus();

private:

	bool bEnablePlayer = true;
	bool isSetup = false; // do we need this?

	atomic<bool> bPlaying = false; // are we currently playing?
	bool bShouldPlay = false;
	atomic<bool> flagPlaybackChange = false;
	set<RTLSSystemType> playingSystems;
	atomic<bool> flagReset = false;

	bool bLoop = true;

	// Queue of takes to play
	queue<RTLSPlayerTake*> takeQueue;

	// Load a take's c3d file into memory
	bool loadTake(RTLSPlayerTake* take);

	// The currently loaded take path
	string takePath = "";
	atomic<float> durationSec = 0;
	atomic<float> fps = 0;

	void paramChanged(RemoteUIServerCallBackArg& arg);

	void threadedFunction();
	std::condition_variable cv;
	atomic<bool> flagUnlock = false;

	// Should the player override realtime data
	// from the same system?
	bool bOverridesRealtimeData = true;

	// TODO: Flag when a file starts again, so
	// filters can be restarted

	ofxTemporalResampler resampler;

	// Get frames from data
	bool getFrames(RTLSPlayerTake* take);
	void sendData(RTLSPlayerTake* take);

	// Let the postprocessors know to reset
	// their filters for the types of this take.
	void notifyResetPostprocessors(RTLSPlayerTake* take);
};

#endif