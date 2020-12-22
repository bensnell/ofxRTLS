#include "ofxRTLS.h"

// --------------------------------------------------------------
ofxRTLS::ofxRTLS() {

}

// --------------------------------------------------------------
ofxRTLS::~ofxRTLS() {

	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLS::setup() {
	ofLogNotice("ofxRTLS") << getSupport();

	// Setup general RTLS params
	//RUI_NEW_GROUP("ofxRTLS");

#ifdef RTLS_PLAYER
	// Setup recorder
	recorder.setup();
	// Setup player
	player.setup();

	// Add a listener for recording started and stopped
	ofAddListener(recorder.recordingEvent, &player, &ofxRTLSPlayer::recordingEvent);
	// Add a listener for play and pause playback
	ofAddListener(player.playbackEvent, &recorder, &ofxRTLSRecorder::playbackEvent);
	// Add a listener for playing back frames
	ofAddListener(player.newPlaybackData, this, &ofxRTLS::playerDataReceived);
#endif
	
#ifdef RTLS_NULL
	// Setup params
	nsys.setup();
	// Add listener for new data
	ofAddListener(nsys.newDataReceived, this, &ofxRTLS::nsysDataReceived);
	
#ifdef RTLS_POSTPROCESS
	nsysPostM.setup(RTLS_SYSTEM_TYPE_NULL, RTLS_TRACKABLE_TYPE_SAMPLE, 
		"NullSysMarkers", "NM");
#ifdef RTLS_PLAYER
	ofAddListener(player.takeLooped, &nsysPostM, &ofxRTLSPostprocessor::resetEventReceved);
#endif
#endif
#endif

#ifdef RTLS_OPENVR
	// Setup Params
	openvr.setup();
	// Add listeners for new data
	ofAddListener(openvr.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	
#ifdef RTLS_POSTPROCESS
	// Setup the postprocessor
	openvrPostM.setup(RTLS_SYSTEM_TYPE_OPENVR, RTLS_TRACKABLE_TYPE_SAMPLE, 
		"OpenVRMarkers", "OM");
#ifdef RTLS_PLAYER
	ofAddListener(player.takeLooped, &openvrPostM, &ofxRTLSPostprocessor::resetEventReceved);
#endif
#endif
#endif

#ifdef RTLS_MOTIVE
	// Setup parameters for Motive
	motive.setupParams();

	// Setup additional parameters
	RUI_NEW_GROUP("ofxMotive Data");
	RUI_SHARE_PARAM_WCN("Motive- Send Camera Data", bSendCameraData);
	RUI_SHARE_PARAM_WCN("Motive- Camera Data Freq", cameraDataFrequency, 0, 300);

	// Add listeners for new data
	ofAddListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);

#ifdef RTLS_POSTPROCESS
	// Setup the postprocessors
	motivePostM.setup(RTLS_SYSTEM_TYPE_MOTIVE, RTLS_TRACKABLE_TYPE_SAMPLE, 
		"MotiveMarkers", "MM", "", 
		"age,axes,kalman,easing,add-rate,continuity,easing");
	motivePostR.setup(RTLS_SYSTEM_TYPE_MOTIVE, RTLS_TRACKABLE_TYPE_OBSERVER,
		"MotiveRef", "MR", "", "axes");
#ifdef RTLS_PLAYER
	ofAddListener(player.takeLooped, &motivePostM, &ofxRTLSPostprocessor::resetEventReceved);
	ofAddListener(player.takeLooped, &motivePostR, &ofxRTLSPostprocessor::resetEventReceved);
#endif
#endif
#endif

	// Add a listener for any new latency measurements
	ofAddListener(latencyCalculated, this, &ofxRTLS::newLatencyCalculated);
	
	startThread();
}

// --------------------------------------------------------------
void ofxRTLS::start() {
	// Start communications with the trackers
#ifdef RTLS_NULL
	nsys.start();
#endif
#ifdef RTLS_OPENVR
	openvr.connect();
#endif
#ifdef RTLS_MOTIVE
	motive.start();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::stop() {
	// Stop communication with the trackers
#ifdef RTLS_NULL
	nsys.stop();
#endif
#ifdef RTLS_OPENVR
	openvr.disconnect();
#endif
#ifdef RTLS_MOTIVE
	motive.stop();
#endif
}

// --------------------------------------------------------------
#ifdef RTLS_NULL

void ofxRTLS::nsysDataReceived(NullSystemEventArgs& args) {
	if (isPlaying(RTLS_SYSTEM_TYPE_NULL)) return;

	uint64_t thisMicros = ofGetElapsedTimeMicros();

	markDataReceived();

	// ==============================================
	// Fake Data
	// Send every frame
	// ==============================================

	ofxRTLSEventArgs outArgs(latencyCalculated);
	outArgs.setStartAssemblyTime(thisMicros);
	outArgs.frame.set_frame_id(nsysFrameID);
	outArgs.frame.set_timestamp(ofGetElapsedTimeMillis());
	outArgs.systemType = args.bOverrideContext ? (RTLSSystemType)args.systemOverride : RTLS_SYSTEM_TYPE_NULL;
	outArgs.trackableType = args.bOverrideContext ? (RTLSTrackableType)args.typeOverride : RTLS_TRACKABLE_TYPE_SAMPLE;
	{
		ofJson js;
		js["s"] = int(outArgs.systemType);
		js["t"] = int(outArgs.trackableType);
		outArgs.frame.set_context(js.dump());
	}

	for (auto& t : args.trackables) {

		Trackable* trackable = outArgs.frame.add_trackables();
		if (t.hasId()) trackable->set_id(t.getId());
		if (t.hasCuid()) trackable->set_cuid(t.getCuid());
		Trackable::Position* position = trackable->mutable_position();
		position->set_x(t.getPosition().x);
		position->set_y(t.getPosition().y);
		position->set_z(t.getPosition().z);
	}

#ifdef RTLS_PLAYER
	// Pass this raw data to the recorder
	recorder.add(RTLS_SYSTEM_TYPE_NULL, nsys.getFrameRate(), outArgs.frame);
	recorder.update(RTLS_SYSTEM_TYPE_NULL);
#endif

	sendData(outArgs);

	nsysFrameID++;
}
#endif

// --------------------------------------------------------------
#ifdef RTLS_OPENVR

void ofxRTLS::openvrDataReceived(ofxOpenVRTrackerEventArgs& args) {
	if (isPlaying(RTLS_SYSTEM_TYPE_OPENVR)) return;

	uint64_t thisMicros = ofGetElapsedTimeMicros();

	markDataReceived();

	// ==============================================
	// Generic Tracker Trackables
	// Send every frame
	// ==============================================

	ofxRTLSEventArgs outArgs(latencyCalculated);
	outArgs.setStartAssemblyTime(thisMicros);
	outArgs.frame.set_frame_id(openvrFrameID);
	outArgs.frame.set_timestamp(ofGetElapsedTimeMillis());
	outArgs.systemType = RTLS_SYSTEM_TYPE_OPENVR;
	outArgs.trackableType = RTLS_TRACKABLE_TYPE_SAMPLE;
	{
		ofJson js;
		js["s"] = int(outArgs.systemType);
		js["t"] = int(outArgs.trackableType);
		outArgs.frame.set_context(js.dump());
	}
		
	for (int i = 0; i < (*args.devices->getTrackers()).size(); i++) {

		Device* tkr = (*args.devices->getTrackers())[i];
		if (tkr->isActive()) {
			Trackable* trackable = outArgs.frame.add_trackables();
			trackable->set_name(tkr->serialNumber);
			Trackable::Position* position = trackable->mutable_position();
			position->set_x(tkr->position.x);
			position->set_y(tkr->position.y);
			position->set_z(tkr->position.z);
			Trackable::Orientation* orientation = trackable->mutable_orientation();
			orientation->set_w(tkr->quaternion.w);
			orientation->set_x(tkr->quaternion.x);
			orientation->set_y(tkr->quaternion.y);
			orientation->set_z(tkr->quaternion.z);
		}
	}

#ifdef RTLS_PLAYER
	// Pass this raw data to the recorder
	recorder.add(RTLS_SYSTEM_TYPE_OPENVR, openvr.getFPS(), outArgs.frame);
	recorder.update(RTLS_SYSTEM_TYPE_OPENVR);
#endif

	sendData(outArgs);

	openvrFrameID++;
}
#endif

// --------------------------------------------------------------
#ifdef RTLS_MOTIVE

void ofxRTLS::motiveDataReceived(MotiveEventArgs& args) {
	if (isPlaying(RTLS_SYSTEM_TYPE_MOTIVE)) return;

	uint64_t thisMicros = ofGetElapsedTimeMicros();

	markDataReceived();

	// ==============================================
	// Marker Trackables
	// Send every frame
	// ==============================================

	// Send each identified marker
	ofxRTLSEventArgs mOutArgs(latencyCalculated);
	mOutArgs.setStartAssemblyTime(thisMicros);
	mOutArgs.frame.set_frame_id(motiveFrameID);
	mOutArgs.frame.set_timestamp(ofGetElapsedTimeMillis());
	mOutArgs.systemType = RTLS_SYSTEM_TYPE_MOTIVE;
	mOutArgs.trackableType = RTLS_TRACKABLE_TYPE_SAMPLE;
	{
		ofJson js;
		js["s"] = int(mOutArgs.systemType);
		js["t"] = int(mOutArgs.trackableType);
		mOutArgs.frame.set_context(js.dump());
	}

	for (int i = 0; i < args.markers.size(); i++) {

		Trackable* trackable = mOutArgs.frame.add_trackables();
		// Instead of setting a char array to the cuid data (which would likely
		// yield invalid utf-8 strings (that can't be json-dumped), set a string
		// equal to "[low bits as uint64_t]-[high bits as uint64_t]".
		string cuid = ofToString(args.markers[i].cuid.LowBits()) + "-" +
			ofToString(args.markers[i].cuid.HighBits());
		trackable->set_cuid(cuid);
		// If the marker is active, set an ID. If passive, don't set the ID.
		if (isMarkerActive(args.markers[i].cuid)) {
			trackable->set_id(getActiveMarkerID(args.markers[i].cuid));
		}
		Trackable::Position* position = trackable->mutable_position();
		position->set_x(args.markers[i].position.x);
		position->set_y(args.markers[i].position.y);
		position->set_z(args.markers[i].position.z);
	}

#ifdef RTLS_PLAYER
	// Pass this raw data to the recorder
	recorder.add(RTLS_SYSTEM_TYPE_MOTIVE, motive.getMaxFPS(), mOutArgs.frame);
#endif

	sendData(mOutArgs);


	// ==============================================
	// Observer Trackables
	// Send with specified period.
	// ==============================================

	uint64_t thisTime = ofGetElapsedTimeMillis();
	if (bSendCameraData && ((lastSendTime == 0) || 
		(thisTime - lastSendTime >= cameraDataFrequency*1000.0))) {
		lastSendTime = thisTime;

		ofxRTLSEventArgs cOutArgs(latencyCalculated);
		cOutArgs.setStartAssemblyTime(thisMicros);
		cOutArgs.frame.set_frame_id(motiveFrameID);
		cOutArgs.frame.set_timestamp(ofGetElapsedTimeMillis());
		cOutArgs.systemType = RTLS_SYSTEM_TYPE_MOTIVE;
		cOutArgs.trackableType = RTLS_TRACKABLE_TYPE_OBSERVER;
		{
			ofJson js;
			js["s"] = int(cOutArgs.systemType);
			js["t"] = int(cOutArgs.trackableType);
			// Passing this information isn't sustainable since it cannot be captured by
			// a recording to a c3d file. If you want to know whether the system needs re-calibration,
			// check the individual cameras to see if any need it.
			//js["m"] = int(args.maybeNeedsCalibration); 
			cOutArgs.frame.set_context(js.dump());
		}

		// Add all cameras (after postprocessing)
		for (int i = 0; i < args.cameras.size(); i++) {

			ofJson js;
			js["m"] = int(args.cameras[i].maybeNeedsCalibration);

			Trackable* trackable = cOutArgs.frame.add_trackables();
			trackable->set_id(args.cameras[i].ID);
			trackable->set_cuid(ofToString(args.cameras[i].serial));
			trackable->set_context(js.dump());
			Trackable::Position* position = trackable->mutable_position();
			position->set_x(args.cameras[i].position.x);
			position->set_y(args.cameras[i].position.y);
			position->set_z(args.cameras[i].position.z);
			Trackable::Orientation* orientation = trackable->mutable_orientation();
			orientation->set_w(args.cameras[i].orientation.w);
			orientation->set_x(args.cameras[i].orientation.x);
			orientation->set_y(args.cameras[i].orientation.y);
			orientation->set_z(args.cameras[i].orientation.z);
		}

#ifdef RTLS_PLAYER
		// Pass this raw data to the recorder
		recorder.add(RTLS_SYSTEM_TYPE_MOTIVE, motive.getMaxFPS(), cOutArgs.frame);
#endif

		sendData(cOutArgs);
	}

#ifdef RTLS_PLAYER
	// Update the recorder
	recorder.update(RTLS_SYSTEM_TYPE_MOTIVE);
#endif
	
	motiveFrameID++;
}
#endif

// --------------------------------------------------------------
#ifdef RTLS_PLAYER
void ofxRTLS::playerDataReceived(ofxRTLSPlayerDataArgs& args) {

	uint64_t thisMicros = ofGetElapsedTimeMicros();

	markDataReceived();

	// ==============================================
	// Playback Data
	// (Only when playback is active)
	// ==============================================

	ofxRTLSEventArgs outArgs(latencyCalculated);
	outArgs.setStartAssemblyTime(thisMicros);
	outArgs.frame.set_timestamp(ofGetElapsedTimeMillis());
	outArgs.frame = args.frame;
	outArgs.systemType = args.systemType;
	outArgs.trackableType = args.trackableType;
	// Context has already been set

	// (Don't record played data)

	// Send the data out appropriately
	sendData(outArgs);
}
#endif

// --------------------------------------------------------------
bool ofxRTLS::sendData(ofxRTLSEventArgs& args) {
	
	switch (args.systemType) {

	// ========================================================================
	case RTLS_SYSTEM_TYPE_NULL: {
#ifdef RTLS_NULL 
		switch (args.trackableType) {

		// --------------------------------------------------------------------
		case RTLS_TRACKABLE_TYPE_SAMPLE: {
#ifdef RTLS_POSTPROCESS
			// Post-process the data, then send it out when ready
			nsysPostM.processAndSend(args, newFrameReceived);
#else
			// Send out data immediately
			ofNotifyEvent(newFrameReceived, args);
#endif
		}; break;

		// --------------------------------------------------------------------
		default: {
			return false; // Invalid trackable type
		}; break;
		}
#else 
		return false; // Not compiled for Null System
#endif
	}; break;

	// ========================================================================
	case RTLS_SYSTEM_TYPE_OPENVR: {
#ifdef RTLS_OPENVR
		switch (args.trackableType) {

		// --------------------------------------------------------------------
		case RTLS_TRACKABLE_TYPE_SAMPLE: {
#ifdef RTLS_POSTPROCESS
			openvrPostM.processAndSend(args, newFrameReceived);
#else
			ofNotifyEvent(newFrameReceived, args);
#endif
		}; break;

		// --------------------------------------------------------------------
		default: {
			return false; // Invalid trackable type
		}; break;
		}
#else
		return false; // Not compiled for OpenVR
#endif
	}; break;

	// ========================================================================
	case RTLS_SYSTEM_TYPE_MOTIVE: {
#ifdef RTLS_MOTIVE
		switch (args.trackableType) {

		// --------------------------------------------------------------------
		case RTLS_TRACKABLE_TYPE_OBSERVER: { // Motive Cameras
#ifdef RTLS_POSTPROCESS
			motivePostR.processAndSend(args, newFrameReceived);
#else
			ofNotifyEvent(newFrameReceived, args);
#endif
		}; break;

		// --------------------------------------------------------------------
		case RTLS_TRACKABLE_TYPE_SAMPLE: { // Motive markers
#ifdef RTLS_POSTPROCESS
			motivePostM.processAndSend(args, newFrameReceived);
#else
			ofNotifyEvent(newFrameReceived, args);
#endif
		}; break;

		// --------------------------------------------------------------------
		default: {
			return false; // Invalid trackable type
		}; break;
		}
#else
		return false; // Not compiled for Motive
#endif
	}; break;

	// ========================================================================
	case RTLS_SYSTEM_TYPE_INVALID:
	default: {
		return false; // Invalid system type
	}; break;
	}

	return true;
}

// --------------------------------------------------------------
void ofxRTLS::threadedFunction() {

	while (isThreadRunning()) {

		uint64_t thisTime = ofGetElapsedTimeMillis();

		// Determine whether we are actively receiving any messages in the last
		// stopGap milliseconds
		bReceivingData = thisTime - lastReceive < stopGap;

		// Determine the frame rate of the data
		mutex.lock();
		while (!dataTimestamps.empty() && dataTimestamps.front() < (thisTime-1000)) {
			dataTimestamps.pop();
		}
		dataFPS = dataFPS * 0.95 + float(dataTimestamps.size()) * 0.05;
		mutex.unlock();
		
		sleep(16);
	}
}

// --------------------------------------------------------------
void ofxRTLS::exit() {

#ifdef RTLS_NULL
#ifdef RTLS_POSTPROCESS
	nsysPostM.exit();
#endif
	ofRemoveListener(nsys.newDataReceived, this, &ofxRTLS::nsysDataReceived);
	nsys.stop();
#endif
#ifdef RTLS_OPENVR
#ifdef RTLS_POSTPROCESS
	openvrPostM.exit();
#endif
	// Remove listener for new device data
	ofRemoveListener(openvr.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	openvr.exit();
#endif
#ifdef RTLS_MOTIVE
#ifdef RTLS_POSTPROCESS
	motivePostM.exit();
	motivePostR.exit();
#endif
	ofRemoveListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif
}

// --------------------------------------------------------------
int ofxRTLS::isConnected() {

	int nConnections = 0;

#ifdef RTLS_NULL
	nConnections += int(nsys.isConnected());
#endif
#ifdef RTLS_OPENVR
	nConnections += int(openvr.isConnected());
#endif
#ifdef RTLS_MOTIVE
	nConnections += int(motive.isConnected());
#endif

	return nConnections;
}

// --------------------------------------------------------------
bool ofxRTLS::isReceivingData() {
	return bReceivingData; // no need to lock for a bool
}

// --------------------------------------------------------------
string ofxRTLS::getSupportedSystems() {

	string out = "{ ";
	string systems = "";
#ifdef RTLS_NULL
	systems += "NULL, ";
#endif
#ifdef RTLS_OPENVR
	systems += "OPENVR, ";
#endif
#ifdef RTLS_MOTIVE
	systems += "MOTIVE, ";
#endif
	if (systems.empty()) systems = "--";
	else systems = systems.substr(0, systems.size() - 2);
	out += systems + " }";
	return out;
}

// --------------------------------------------------------------
bool ofxRTLS::isPostprocessSupported() {
#ifdef RTLS_POSTPROCESS
	return true;
#else
	return false;
#endif
}

// --------------------------------------------------------------
bool ofxRTLS::isPlayerSupported() {
#ifdef RTLS_PLAYER
	return true;
#else
	return false;
#endif
}

// --------------------------------------------------------------
string ofxRTLS::getSupport() {
	string supportStr = "ofxRTLS compiled to support systems ";
	supportStr += getSupportedSystems();
	supportStr += " ";

	supportStr += "with postprocessing ";
	supportStr += isPostprocessSupported() ? "ENABLED" : "DISABLED";
	supportStr += " ";
	
	supportStr += "and player ";
	supportStr += isPlayerSupported() ? "ENABLED" : "DISABLED";
	
	supportStr += ".";
	return supportStr;
}

// --------------------------------------------------------------
float ofxRTLS::getMaxSystemFPS() {

	float maxFPS = 0;
#ifdef RTLS_NULL
	maxFPS = max(maxFPS, nsys.getFrameRate());
#endif
#ifdef RTLS_OPENVR
	maxFPS = max(maxFPS, openvr.getFPS());
#endif
#ifdef RTLS_MOTIVE
	maxFPS = max(maxFPS, float(motive.getMaxFPS()));
#endif
	return maxFPS;
}

// --------------------------------------------------------------
void ofxRTLS::newLatencyCalculated(ofxRTLSLatencyArgs& args) {

	double latency = double(args.stopTimeUS - args.startTimeUS)/1000.0;
	latencyMS = latencyMS * 0.95 + latency * 0.05;
}

// --------------------------------------------------------------
bool ofxRTLS::isRecording() {
#ifdef RTLS_PLAYER
	return recorder.isRecording();
#endif
	return false;
}

// --------------------------------------------------------------
bool ofxRTLS::isSavingRecording() {
#ifdef RTLS_PLAYER
	return recorder.isSaving();
#endif
	return false;
}

// --------------------------------------------------------------
float ofxRTLS::getSavingRecordingPercentComplete() {
#ifdef RTLS_PLAYER
	return recorder.getSavingPercentageComplete();
#endif
	return 0;
}

// --------------------------------------------------------------
bool ofxRTLS::isPlaying() {
#ifdef RTLS_PLAYER
	return player.isPlaying();
#endif
	return false;
}

// --------------------------------------------------------------
string ofxRTLS::getRecordingFile() {
#ifdef RTLS_PLAYER
	return recorder.getRecordingFile();
#endif
	return "";
}

// --------------------------------------------------------------
string ofxRTLS::getPlayingFile() {
#ifdef RTLS_PLAYER
	return player.getTakePath();
#endif
	return "";
}

// --------------------------------------------------------------
void ofxRTLS::markDataReceived() {

	lastReceive = ofGetElapsedTimeMillis();
	mutex.lock();
	dataTimestamps.push(lastReceive);
	mutex.unlock();
}

// --------------------------------------------------------------
bool ofxRTLS::isPlaying(RTLSSystemType systemType) {
#ifdef RTLS_PLAYER
	return player.isPlaying(systemType);
#endif
	return false;
}

// --------------------------------------------------------------
void ofxRTLS::toggleRecording() {
#ifdef RTLS_PLAYER
	recorder.toggleRecording();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::toggleRecordingWithSavePrompt() {
#ifdef RTLS_PLAYER
	recorder.toggleRecordingWithSavePrompt();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::togglePlayback() {
#ifdef RTLS_PLAYER
	player.togglePlayback();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::resetPlayback() {
#ifdef RTLS_PLAYER
	player.reset();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::promptOpenPlaybackFile() {
#ifdef RTLS_PLAYER
	player.promptUserOpenFile();
#endif
}

// --------------------------------------------------------------
float ofxRTLS::getPlayingPercentComplete() {
#ifdef RTLS_PLAYER
	return player.getTakePercentComplete();
#endif
	return 0;
}

// --------------------------------------------------------------
float ofxRTLS::getPlayingFileDuration() {
#ifdef RTLS_PLAYER
	return player.getTakeDuration();
#endif
	return 0;
}

// --------------------------------------------------------------
float ofxRTLS::getRecordingDuration() {
#ifdef RTLS_PLAYER
	return recorder.getRecordingDuration();
#endif
	return 0;
}

// --------------------------------------------------------------
