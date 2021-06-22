#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRTLSEventArgs.h"
#include "ofxRTLSTypes.h"
#include "ofxRTLSTake.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;
#include "ofxRTLSTrackableKey.h"

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
	uint64_t getTakeNumFrames() { return numFrames; }
	float getTakePercentComplete();
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
	void recordingEvent(ofxRTLSRecordingArgs& args);

	// This event is called when the player loops
	ofEvent<ofxRTLSPlayerLoopedArgs> takeLooped;

	// This event is notified when new data is available from the player.
	ofEvent<ofxRTLSPlayerDataArgs> newPlaybackData;

	// This event is notified when we pause or play
	ofEvent<ofxRTLSPlaybackArgs> playbackEvent;

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

	// Start and stop times for playback window (in seconds).
	// These variables are connected to RUI. We cannot trust that they won't
	// change during program execution.
	float windowStartTime = 0;
	float windowStopTime = 1;
	// Validate the window size, checking to make sure the start
	// time is less than the stop time, and copy these values into the variables below,
	// which serve as a single-source of truth while executing a loop 
	// in this thread.
	void validateWindow(RTLSPlayerTake* take);
	uint64_t windowStartFrame = 0;
	uint64_t windowStopFrame = 0;
	uint64_t windowNumFrames = 0;

	// Check if the frame counter exists outside of the window
	bool frameExceedsWindow(uint64_t counter);



	// Queue of takes to play
	queue<RTLSPlayerTake*> takeQueue;

	// Load a take's c3d file into memory
	bool loadTake(RTLSPlayerTake* take);

	// The currently loaded take parameters
	// (These parameters are not the single source of truth; 
	// they exist solely for the purposes of being
	// accessible to external threads at any point in time.)
	string takePath = "";
	atomic<float> durationSec = 0;
	atomic<float> fps = 0;
	atomic<uint64_t> numFrames = 0;
	atomic<uint64_t> frameCounter = 0;

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
	// Allow certain types of data through
	struct Allow {
		bool allow = true;
	};
	vector<Allow> allowSystemTypes;
	vector<Allow> allowTrackableTypes;

	// Let the postprocessors know to reset
	// their filters for the types of this take.
	void notifyResetPostprocessors(RTLSPlayerTake* take);
};