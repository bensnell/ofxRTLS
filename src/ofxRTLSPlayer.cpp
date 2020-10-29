#include "ofxRTLSPlayer.h"

#ifdef RTLS_PLAYER

// --------------------------------------------------------------
ofxRTLSPlayer::ofxRTLSPlayer() {

}

// --------------------------------------------------------------
ofxRTLSPlayer::~ofxRTLSPlayer() {

	// Stop playing
	pause();

	// Flag that we should stop waiting 
	flagUnlock = true;
	
	// Signal the conditional variable
	cv.notify_one();

	// Stop this thread and wait for it to complete
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSPlayer::setup() {

	RUI_NEW_GROUP("ofxRTLSPlayer");
	RUI_SHARE_PARAM_WCN("RTLS-P- Enable", bEnablePlayer);
	RUI_SHARE_PARAM_WCN("RTLS-P- Play", bShouldPlay);
	RUI_SHARE_PARAM_WCN("RTLS-P- Take Path", takePath);
	RUI_SHARE_PARAM_WCN("RTLS-P- Loop", bLoop);
	RUI_SHARE_PARAM_WCN("RTLS-P- Override Realtime Data", bOverridesRealtimeData);

	bShouldPlay = false;
	bPlaying = false;
	RUI_PUSH_TO_CLIENT();

	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRTLSPlayer::paramChanged);

	isSetup = true;

	startThread();

	// If the takePath isn't empty, then add this take to the queue
	if (!takePath.empty()) setPlayingFile(takePath);
}

// --------------------------------------------------------------
void ofxRTLSPlayer::threadedFunction() {

	// This is the currently loaded take, if any
	RTLSPlayerTake* take = NULL;

	while (isThreadRunning()) {

		bool bWakeup = false;
		{
			// Lock the mutex
			std::unique_lock<std::mutex> lk(mutex);

			// This locks the mutex (if not already locked) in order to check
			// the predicate (whether the queue contains items). If false, the mutex 
			// is unlocked and waits for the condition variable to receive a signal
			// to check again. If true, code execution continues.
			cv.wait(lk, [this] { return flagUnlock || !takeQueue.empty() || flagPlaybackChange; });

			if (!flagUnlock) bWakeup = true;
		}
		// If we're not playing a take, continue.
		if (!bWakeup) continue;

		// Reset the playback flag
		flagPlaybackChange = false;

		// Reset the dynamic fps resampler
		resampler.reset();

		// Load the next take
		//take = NULL; // delete remaining takes? // take should be null
		while (true) {

			// Check for new takes in the queue.
			RTLSPlayerTake* nextTake = NULL;
			{
				std::lock_guard<std::mutex> lk(mutex);
				while (!takeQueue.empty()) {
					nextTake = takeQueue.front();
					takeQueue.pop();
				}
			}
			// If there is a new take ...
			if (nextTake != NULL) {

				// ... then unload the old one, if any, ...
				if (take != NULL) {
					// Stop playing, if playing
					bPlaying = false;
					// Clear the take
					take->clear();
					delete take;
					take = NULL;
				}

				// ... and load this one.
				take = nextTake;
				nextTake = NULL;
				if (loadTake(take)) { // Success loading
					ofLogNotice("ofxRTLSPlayer") << "Loaded take \"" << take->path << "\"";
					// Set new take parameters
					string newPath = "";
					{
						std::lock_guard<std::mutex> lk(mutex);
						newPath = take->path;
						durationSec = take->getC3dDurationSec();
						fps = take->getC3dFps();
						resampler.setDesiredFPS(fps);
					}
					if (newPath.compare(takePath) != 0) {
						takePath = newPath;
						RUI_PUSH_TO_CLIENT();
					}
				}
				else { // Failure loading
					take->clear();
					delete take;
					take = NULL;
				}
			}

			// If there is no take loaded, then pause and break
			if (take == NULL) {
				bPlaying = false;
				bShouldPlay = false;
				RUI_PUSH_TO_CLIENT();
				break;
			}

			// Does this take have any points in it? If not, don't play it
			if (take->getC3dNumFrames() == 0) {
				ofLogNotice("ofxRTLSPlayer") << "No data in take \"" << take->path << "\"";
				bPlaying = false;
				break;
			}

			// At this point, we know we have a valid, loaded take.
			// Check if we should start or stop playing.
			// Also check if we're looping.
			bool _bLoop = false;
			{
				std::lock_guard<std::mutex> lk(mutex);
				bPlaying = bShouldPlay;
				_bLoop = bLoop;
			}
			// If we're not playing, then break from this loop.
			if (!bPlaying) break;

			// Check if we aren't looping and need to stop,
			// since we've reached the end of the file.
			// Also check if we have a signal to stop playing and reset the
			// frame counter.
			if ((!_bLoop && take->frameCounter >= take->getC3dNumFrames())
				|| flagReset) {

				// Reset the take
				flagReset = false;
				bPlaying = false;
				take->frameCounter = 0;

				// Signal that filters need to be reset
				notifyResetPostprocessors(take);
			}
			// If we're not playing, then break from this loop.
			if (!bPlaying) break;

			// If this frame is zero, reset the postprocessors
			if (take->frameCounter == 0) notifyResetPostprocessors(take);

			// At this point, we have a valid take and are playing.
			// Attempt to read the next frame and send it.
			if (getFrames(take)) {
				sendData(take);
			}

			// Increment the frame counter
			take->frameCounter++;
			if (_bLoop && take->getC3dNumFrames() > 0) {
				take->frameCounter = take->frameCounter % take->getC3dNumFrames();
			}
			// TODO: If we loop, signal that filters need to be reset
			
			// Update the fps resampler
			resampler.update();
			// Sleep according to the resampler
			sleep(resampler.getSleepDurationMS());
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSPlayer::setLooping(bool _bLoop) {
	
	if (bLoop == _bLoop) return;
	bLoop = _bLoop;
	RUI_PUSH_TO_CLIENT();
}

// --------------------------------------------------------------
void ofxRTLSPlayer::play() {
	if (!isSetup) return;

	if (!bPlaying) {
		bShouldPlay = true;
		RUI_PUSH_TO_CLIENT();

		// Notify that we are starting to play
		ofxRTLSPlaybackArgs args;
		args.bPlay = true;
		ofNotifyEvent(playbackEvent, args);

		flagPlaybackChange = true;
		cv.notify_one();
	}
}

// --------------------------------------------------------------
void ofxRTLSPlayer::pause() {
	if (!isSetup) return;

	if (bPlaying) {
		bShouldPlay = false;
		RUI_PUSH_TO_CLIENT();

		// Notify that we have paused
		ofxRTLSPlaybackArgs args;
		args.bPause = true;
		ofNotifyEvent(playbackEvent, args);

		// No need to notify thread, because it is already active
		// (and playing).
		//flagPlaybackChange = true;
		//cv.notify_one();
	}
}

// --------------------------------------------------------------
void ofxRTLSPlayer::reset() {
	if (!isSetup) return;

	flagReset = true;
	flagPlaybackChange = true;
	cv.notify_one();
}

// --------------------------------------------------------------
void ofxRTLSPlayer::togglePlayback() {

	if (bPlaying) pause();
	else play();
}

// --------------------------------------------------------------
void ofxRTLSPlayer::setOverrideRealtimeData(bool _bOverride) {

	if (bOverridesRealtimeData != _bOverride) {
		bOverridesRealtimeData = _bOverride;
		RUI_PUSH_TO_CLIENT();
	}
}

// --------------------------------------------------------------
string ofxRTLSPlayer::getStatus() {

	stringstream ss;
	if (!isSetup) ss << "Player not setup";
	else {
		if (!bEnablePlayer) ss << "Player is DISABLED";
		else {
			if (!bPlaying) ss << "Playing OFF";
			else {
				ss << "Playing file " << takePath;
			}
		}
	}
	return ss.str();
}

// --------------------------------------------------------------
void ofxRTLSPlayer::paramChanged(RemoteUIServerCallBackArg& arg) {
	if (!isSetup) return;
	if (!arg.action == CLIENT_UPDATED_PARAM) return;
	
	if (arg.paramName.compare("RTLS-P- Play") == 0) {
		if (bShouldPlay && !bPlaying) play();
		else if (!bShouldPlay && bPlaying) pause();
	}
	if (arg.paramName.compare("RTLS-P- Take Path") == 0) {
		setPlayingFile(takePath);
	}
}

// --------------------------------------------------------------
bool ofxRTLSPlayer::loadTake(RTLSPlayerTake* take) {

	// Verify the validity of this take
	if (take == NULL) return false;
	if (take->path.empty()) return false;

	// Attempt to load the c3d file
	bool bSuccess = false;
	try {
		if (take->c3d != NULL) { // should not get to this point...
			take->c3d->~c3d();
			delete take->c3d;
		}
		take->c3d = new ezc3d::c3d(take->path);
		bSuccess = true;
	}
	catch (const std::exception&) {
		ofLogError("ofxRTLSPlayer") << "Could not read c3d file \"" << take->path << "\"";
	}
	if (!bSuccess) return false;

	// Make sure this c3d file has been generated by RTLS and
	// not another program.
	// Parameters: MANUFACTURER > SOFTWARE > RTLSServer
	vector<string> values(take->c3d->parameters().group("MANUFACTURER").parameter("SOFTWARE").valuesAsString());
	if (values.empty()) return false;
	string software = ofTrim(ofToLower(values.front()));
	if (software.compare("rtlsserver") != 0) {
		ofLogError("ofxRTLSPlayer") << "Cannot play a c3d file generated by a different utility.";
		return false;
	}

	// Try to populate the take with template frames
	bSuccess = false;
	try {
		bSuccess = take->populateTemplateFrames();
	}
	catch (const std::exception&) {
		bSuccess = false;
	}
	if (!bSuccess) {
		ofLogError("ofxRTLSPlayer") << "Could not parse take's data.";
	}
	// Store all systems that will be playing
	if (bSuccess) {
		std::lock_guard<std::mutex> lk(mutex);
		playingSystems.clear();
		for (auto& frame : take->frames) {
			playingSystems.insert(frame.systemType);
		}
	}

	return bSuccess;
}

// --------------------------------------------------------------
void ofxRTLSPlayer::promptUserOpenFile() {

	ofFileDialogResult result = ofSystemLoadDialog("Select a .c3d file to playback", false, ofFilePath::getCurrentExeDir());
	if (!result.bSuccess) return;

	setPlayingFile(result.filePath);
}

// --------------------------------------------------------------
void ofxRTLSPlayer::setPlayingFile(string filePath) {

	// Remove whitespace
	filePath = ofTrim(filePath);

	// Get the absolute path
	if (!ofFilePath::isAbsolute(filePath)) {
		filePath = ofToDataPath(filePath, true);
	}

	// Make sure the file exists
	if (!ofFile::doesFileExist(filePath)) {
		ofLogNotice("ofxRTLSPlayer") << "File does not exist: \"" << filePath << "\"";
		return;
	}

	// Make sure it has the correct extension
	string ext = ofToLower(ofFilePath::getFileExt(filePath));
	if (ext.compare("c3d") != 0) {
		ofLogNotice("ofxRTLSPlayer") << "Please provide a path to a .c3d file.";
		return;
	}

	// Queue this take to be loaded
	RTLSPlayerTake* take = new RTLSPlayerTake();
	take->path = filePath;
	takeQueue.push(take);

	// Notify that we need to load a new take at the next
	// available opportunity, and pause/unload anything currently
	// playing or loaded.
	cv.notify_one();
}

// --------------------------------------------------------------
void ofxRTLSPlayer::recordingEvent(ofxRTLSRecordingArgs& args) {

	if (args.bRecordingEnded) {
		// Add this file to the queue. It will be loaded at the next
		// available chance. Any file currently loaded will be 
		// unloaded
		setPlayingFile(args.filePath);
	}
	else if (args.bRecordingBegan) {
		// Stop any take that is currently playing
		pause();
	}
}

// --------------------------------------------------------------
bool ofxRTLSPlayer::getFrames(RTLSPlayerTake* take) {
	if (!isSetup) return false;
	if (take == NULL) return false;
	if (take->c3d == NULL) return false;

	// Flag all frames as old
	take->flagAllFramesOld();

	// Get points for this take
	auto pts = take->c3d->data().frame(take->frameCounter).points();
	// If there are no points, then don't proceed
	if (pts.isEmpty()) return false;

	// Fill all newFrames with data, where available
	for (auto& _f : take->frames) {

		auto& frame = _f.newFrame;
		auto& refFrame = _f.frame;

		// Prepare the frame for incoming data
		frame.Clear();
		frame.CopyFrom(refFrame);
		frame.clear_trackables();

		// Set all relevant points, filling the frame with data from the c3d file
		for (int index = 0; index < _f.dataIndices.size(); index++) {
			// "index" indicates the index of a trackable in the refFrame
			// "ptIndex" will indicate the corresponding point in the c3d file's list of points
			int ptIndex = _f.dataIndices[index];
			if (pts.point(ptIndex).isValid()) {
				Trackable* tk = frame.add_trackables();
				tk->CopyFrom(refFrame.trackables(index));
				Trackable::Position* position = tk->mutable_position();
				position->set_x(pts.point(ptIndex).x());
				position->set_y(pts.point(ptIndex).y());
				position->set_z(pts.point(ptIndex).z());
				_f.bNewData = true;
			}
		}

		// Set the frame ID
		frame.set_frame_id(take->frameCounter);
	}

	return true;
}

// --------------------------------------------------------------
void ofxRTLSPlayer::sendData(RTLSPlayerTake* take) {
	if (!isSetup) return;
	if (take == NULL) return;

	// Send every frame
	for (int i = 0; i < take->frames.size(); i++) {

		// Only send frames with new data
		if (!take->frames[i].bNewData) continue;

		// Copy over data and send it
		ofxRTLSPlayerDataArgs args;
		args.frame.CopyFrom(take->frames[i].newFrame);
		args.systemType = take->frames[i].systemType;
		args.trackableType = take->frames[i].trackableType;
		ofNotifyEvent(newPlaybackData, args);
	}
}

// --------------------------------------------------------------
bool ofxRTLSPlayer::isPlaying(RTLSSystemType systemType) {
	if (!bPlaying) return false;

	std::lock_guard<std::mutex> lk(mutex);
	return playingSystems.find(systemType) != playingSystems.end();
}

// --------------------------------------------------------------
void ofxRTLSPlayer::notifyResetPostprocessors(RTLSPlayerTake* take) {

	ofxRTLSPlayerLoopedArgs args;
	for (auto& f : take->frames) {
		args.systems.push_back(make_pair(f.systemType, f.trackableType));
	}
	ofNotifyEvent(takeLooped, args);
}

// --------------------------------------------------------------

// --------------------------------------------------------------

#endif