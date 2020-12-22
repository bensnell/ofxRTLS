#include "ofxRTLSRecorder.h"

#ifdef RTLS_PLAYER

// --------------------------------------------------------------
ofxRTLSRecorder::ofxRTLSRecorder() {

}

// --------------------------------------------------------------
ofxRTLSRecorder::~ofxRTLSRecorder() {

	// Flag that we should stop waiting 
	flagUnlock = true;
	// Signal the conditional variable
	cv.notify_one();

	// Stop this thread and wait for it to complete
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSRecorder::setup(string _takeFolder, string _takePrefix) {
	
	if (!_takeFolder.empty()) takeFolder = _takeFolder;
	if (!_takePrefix.empty()) takePrefix = _takePrefix;

	RUI_NEW_GROUP("ofxRTLSRecorder");
	RUI_SHARE_PARAM_WCN("RTLS-R- Enable", bEnableRecorder);
	RUI_SHARE_PARAM_WCN("RTLS-R- Record", bShouldRecord);
	RUI_SHARE_PARAM_WCN("RTLS-R- Take Folder", takeFolder);
	RUI_SHARE_PARAM_WCN("RTLS-R- Take Prefix", takePrefix);

	bShouldRecord = false;
	bRecording = false;
	RUI_PUSH_TO_CLIENT();

	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRTLSRecorder::paramChanged);

	isSetup = true;

	startThread();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::threadedFunction() {

	RTLSTake* take = NULL;
	while (isThreadRunning()) {

		bool bSaveTake = false;
		{
			// Lock the mutex
			std::unique_lock<std::mutex> lk(mutex);

			// This locks the mutex (if not already locked) in order to check
			// the predicate (whether the queue contains items). If false, the mutex 
			// is unlocked and waits for the condition variable to receive a signal
			// to check again. If true, code execution continues.
			cv.wait(lk, [this] { return flagUnlock || !takeQueue.empty(); });

			if (!flagUnlock) bSaveTake = true;
		}

		// If we're not saving a take, continue.
		if (!bSaveTake) continue;

		// Save all takes ready to be saved
		while (true) {

			// Check if this take is ready to be saved. If so, pop it.
			take = NULL;
			{
				std::lock_guard<std::mutex> lk(mutex);
				// If there are no more takes, break.
				if (takeQueue.empty()) break;
				// Check if the next take should be saved.
				if (takeQueue.front()->bFlagSave) {
					// If so, retrieve it and remove it from the queue.
					take = takeQueue.front();
					takeQueue.pop();
				}
				else {
					if (takeQueue.size() > 1) {
						ofLogError("ofxRTLSRecorder") << "Take queue has more than 2 currently recording takes. This should not be happening.";
					}
				}
			}

			// If the take is null, break from the loop
			if (take == NULL) break;

			// Save the take to file
			bool bTakeSaved = false;
			try {
				bTakeSaved = saveTake(take);
			}
			catch (const std::exception&) {
				ofLogError("ofxRTLSRecorder") << "Encountered error while trying to save the take.";
			}
			if (bTakeSaved) {
				ofLogNotice("ofxRTLSRecorder") << "Saved take to file \"" << take->path << "\"" << endl;

				// Also notify that we successfully saved it
				ofxRTLSRecordingArgs args;
				args.bRecordingEnded = true;
				args.filePath = take->path;
				ofNotifyEvent(recordingEvent, args);
			} else
			{
				ofLogNotice("ofxRTLSRecorder") << "Could not save take to file \"" << take->path << "\"" << endl;
			}

			// Delete the take
			take->clear();
			delete take;
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSRecorder::add(int systemIndex, float systemFPS, RTLSProtocol::TrackableFrame& _frame) {
	if (!isSetup) return;
	if (!bEnableRecorder) return;
	if (!bRecording) return;
	if (systemFPS <= 0) return;

	// Add the element to the appropriate recording
	std::lock_guard<std::mutex> lk(mutex);

	// Only add the frame if there is a take in the queue (this shouldn't need to be checked, but
	// let's do it for safety).
	if (takeQueue.empty()) return;

	// Get the last take
	auto take = takeQueue.back();

	// Ensure this system has been approved by the take
	if (!take->systemExists(systemIndex)) {
		// If not, will it?
		if (take->fps < 0) take->fps = systemFPS;
		else if (abs(take->fps - systemFPS) > 0.001) {
			ofLogWarning("ofxRTLSRecorder") << "Cannot record from multiple systems at once that have different frame rates";
			return;
		}
		// Take can accept system, so add it
		take->data[systemIndex] = RTLSTake::RTLSTakeSystemData();
	}

	// If this is the first frame, set the start time
	if (take->startTimeMS == 0) take->startTimeMS = ofGetElapsedTimeMillis();

	// Get the system data
	auto& data = take->data[systemIndex];

	// Add this frame
	RTLSProtocol::TrackableFrame* frame = new RTLSProtocol::TrackableFrame(_frame);
	data.nextFrame.push_back(frame);
}

// --------------------------------------------------------------
void ofxRTLSRecorder::update(int systemIndex) {
	if (!isSetup) return;
	if (!bEnableRecorder) return;
	if (!bRecording) return;

	// Add the element to the appropriate recording
	std::lock_guard<std::mutex> lk(mutex);

	// Only add the frame if there is a take in the queue (this shouldn't need to be checked, but
	// let's do it for safety).
	if (takeQueue.empty()) return;

	// Get the last take
	auto take = takeQueue.back();

	// Make sure this system is present in the recording
	if (!take->systemExists(systemIndex)) return;

	// Get this system data
	auto& data = take->data[systemIndex];

	// Include all trackable keys in the label set
	for (auto& frame : data.nextFrame) {
		for (int i = 0; i < frame->trackables_size(); i++) {
			auto& tk = frame->trackables(i);
			string key = getTrackableKey(tk);
			auto ret = take->c3dPointLabels.insert(key);
			
			// Add a description if it is a new element
			if (ret.second) {

				// Parse the frame-specific context
				ofJson frameContext;
				try {
					frameContext = ofJson::parse(frame->context());
				}
				catch (const std::exception&) {
					// Could not parse
				}
				
				// Parse the trackable-specific context
				ofJson trackableContext;
				try {
					trackableContext = ofJson::parse(frame->trackables(i).context());
				}
				catch (const std::exception&) {
					// Could not parse
				}

				// Add all information to the descriptions
				// TODO: Allow both trackable and frame context to pass calibration flags 'm'
				ofJson js;
				if (frameContext.find("s") != frameContext.end()) js["frame"]["context"]["s"] = frameContext["s"];
				if (frameContext.find("t") != frameContext.end()) js["frame"]["context"]["t"] = frameContext["t"];
				if (!trackableContext.empty()) js["trackable"]["context"] = trackableContext;				
				if (!tk.name().empty()) js["trackable"]["name"] = tk.name();
				if (!tk.cuid().empty()) js["trackable"]["cuid"] = tk.cuid();
				if (tk.id() != 0) js["trackable"]["id"] = tk.id();
				take->c3dPointLabels2Desc[key] = js.dump();
			}
		}
	}

	// Add the vector of frames to the queue and clear the vector
	data.addNextFrame();
}

// --------------------------------------------------------------
string ofxRTLSRecorder::getStatus() {

	stringstream ss;
	if (!isSetup) ss << "Recorder not setup";
	else {
		if (!bEnableRecorder) ss << "Recorder is DISABLED";
		else {
			if (!bRecording) ss << "Recording OFF";
			else {
				ss << "Recording to file " << thisTakePath;
				ss << setprecision(2);
				ss << " [" << float(ofGetElapsedTimeMillis() - thisTakeStartTimeMS) / 1000.0 << " sec]";
			}
		}
	}
	return ss.str();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::paramChanged(RemoteUIServerCallBackArg& arg) {
	if (!isSetup) return;
	if (!arg.action == CLIENT_UPDATED_PARAM) return;
	
	if (arg.paramName.compare("RTLS-R- Record") == 0) {
		if (bShouldRecord && !bRecording) {
			
			// Begin new recording
			RTLSTake* take = new RTLSTake();
			// Check if user supplied a path
			{
				lock_guard<ofMutex> guard(userSavePathMutex);
				thisTakePath = (userSavePath.empty() ? generateTakePath() : userSavePath);
				userSavePath = "";
			}			
			thisTakeStartTimeMS = ofGetElapsedTimeMillis();
			take->path = thisTakePath;

			{
				std::lock_guard<std::mutex> lk(mutex);
				takeQueue.push(take);
				bRecording = true;
			}

			// Notify that we started a recording
			ofxRTLSRecordingArgs args;
			args.bRecordingBegan = true;
			args.filePath = take->path;
			ofNotifyEvent(recordingEvent, args);
		}
		else if (!bShouldRecord && bRecording) {

			// Stop recording
			bRecording = false;

			// If the user selected a new path to save to, get it
			string pathOverride = "";
			{
				lock_guard<ofMutex> guard(userSavePathMutex);
				pathOverride = userSavePath;
				userSavePath = "";
			}

			// Flag that we should save this take
			std::lock_guard<std::mutex> lk(mutex);
			if (!pathOverride.empty()) {
				thisTakePath = pathOverride;
				takeQueue.back()->path = pathOverride;
			}
			takeQueue.back()->bFlagSave = true;

			// Notify that the condition has been met to save the file
			cv.notify_one();
		}
		else if (!bShouldRecord) {

			// Notify anyway
			cv.notify_one();
		}
	}
}

// --------------------------------------------------------------
bool ofxRTLSRecorder::saveTake(RTLSTake* take) {

	// Verify the validity of this take
	if (take == NULL) {
		ofLogError("ofxRTLSRecorder") << "Cannot save NULL take.";
		return false;
	}
	if (take->empty()) {
		ofLogError("ofxRTLSRecorder") << "Cannot save empty take.";
		return false;
	}
	if (!take->valid()) {
		ofLogError("ofxRTLSRecorder") << "Cannot save invalid take.";
		return false;
	}

	// Flag that we have begun saving
	takeSavingFramesTotal = take->getNumFrames();
	takeSavingFramesSaved = 0;
	isTakeSaving = true;
	
	// Proceed with saving the take
	auto& c3d = take->c3d;

	// Add all point labels
	for (auto& label : take->c3dPointLabels) {
		c3d.point(label);
	}

	// Create a map from point labels to their index
	map<string, int> c3dPointLabels2Index;
	uint64_t counter = 0;
	for (auto& label : take->c3dPointLabels) {
		c3dPointLabels2Index[label] = counter;
		counter++;
	}
	
	// Set the recording's properties, after adding the points.

	// Point Properties
	ezc3d::ParametersNS::GroupNS::Parameter pointRate("RATE");
	pointRate.set(take->fps);
	c3d.parameter("POINT", pointRate);
	ezc3d::ParametersNS::GroupNS::Parameter pointScale("SCALE");
	pointScale.set(-1.0);
	c3d.parameter("POINT", pointScale);
	ezc3d::ParametersNS::GroupNS::Parameter pointUnits("UNITS");
	pointUnits.set("m");
	c3d.parameter("POINT", pointUnits);
	ezc3d::ParametersNS::GroupNS::Parameter pointDescriptions("DESCRIPTIONS");
	vector<string> pointDesc;
	for (auto& label : take->c3dPointLabels) {
		if (take->c3dPointLabels2Desc.find(label) != take->c3dPointLabels2Desc.end()) {
			pointDesc.push_back(take->c3dPointLabels2Desc[label]);
		}
		else {
			pointDesc.push_back("");
		}
	}
	pointDescriptions.set(pointDesc);
	c3d.parameter("POINT", pointDescriptions);

	// Manufacturer Properties
	ezc3d::ParametersNS::GroupNS::Parameter mfrCompany("COMPANY");
	mfrCompany.set("Local Projects");
	c3d.parameter("MANUFACTURER", mfrCompany);
	ezc3d::ParametersNS::GroupNS::Parameter mfrSoftware("SOFTWARE");
	mfrSoftware.set("RTLSServer");
	c3d.parameter("MANUFACTURER", mfrSoftware);
	ezc3d::ParametersNS::GroupNS::Parameter mfrSoftwareDesc("SOFTWARE_DESCRIPTION");
	mfrSoftwareDesc.set("Real Time Location System Server");
	c3d.parameter("MANUFACTURER", mfrSoftwareDesc);
	ezc3d::ParametersNS::GroupNS::Parameter mfrVersion("VERSION_LABEL");
	mfrVersion.set("1.0"); // arbitrary
	c3d.parameter("MANUFACTURER", mfrVersion);

	
	// Add all points
	// NAN will stand for absent points
	int nPoints = take->c3dPointLabels.size();
	while (!take->empty()) {

		// Create a new c3d frame 
		ezc3d::DataNS::Frame frame;

		// Create points and populate each with NANs
		ezc3d::DataNS::Points3dNS::Points points;
		for (int i = 0; i < nPoints; i++) {
			ezc3d::DataNS::Points3dNS::Point point;
			point.set(NAN, NAN, NAN);
			points.point(point);
		}

		// Iterate through all contributing systems
		map<int, RTLSTake::RTLSTakeSystemData>::iterator sys;
		for (sys = take->data.begin(); sys != take->data.end(); sys++) {

			// Get the queue of frames. Each frame is a vector of TrackableFrames (TF's), since
			// one system can output multiple TF's each frame (for example, Motive can export
			// both observer and marker points each frame).
			queue<vector<RTLSProtocol::TrackableFrame*>>& frameQueue = sys->second.frames;
			// If the frame is empty, then there are no points left to add.
			if (frameQueue.empty()) continue;
			
			// Get the next set of trackable frames
			vector<RTLSProtocol::TrackableFrame*>& frameSet = frameQueue.front();
			// Iterate through all of these frames
			RTLSProtocol::TrackableFrame* tkFrame;
			for (int frameIndex = 0; frameIndex < frameSet.size(); frameIndex++) {
				tkFrame = frameSet[frameIndex];

				// For each frame, iterate through all of its trackables
				for (int i = 0; i < tkFrame->trackables_size(); i++) {
					
					// Get the next trackable in this TF
					auto& tk = tkFrame->trackables(i);

					// Get the key and access the point corresponding with this key.
					string key = getTrackableKey(tk);
					auto& point = points.point(c3dPointLabels2Index[key]); // check for valid index?
					// Set the position
					point.set(
						tk.position().x(), 
						tk.position().y(), 
						tk.position().z());
				}
			}

			// Pop the front value that we just processed from the queue.
			sys->second.clearAndPopNextFrame();
		}

		// Add the points to the frame
		frame.add(points);

		// Add the frame to the take
		c3d.frame(frame);

		// Increment the number of frames saved
		++takeSavingFramesSaved;
	}
	
	// Save the c3d to file
	ofFilePath::createEnclosingDirectory(take->path);
	c3d.write(take->path);

	// Flag that we are done saving
	isTakeSaving = false;

	return true;
}

// --------------------------------------------------------------
void ofxRTLSRecorder::toggleRecording() {

	// TODO: Make this safer

	bShouldRecord = !bRecording;
	RUI_PUSH_TO_CLIENT();

	RemoteUIServerCallBackArg arg;
	arg.action = CLIENT_UPDATED_PARAM;
	arg.paramName = "RTLS-R- Record";
	paramChanged(arg);
}

// --------------------------------------------------------------
void ofxRTLSRecorder::toggleRecordingWithSavePrompt()
{
	// Attempt to get a path to save this recording as
	auto result = ofSystemSaveDialog(generateTakePath(), "Save recording as");
	if (result.bSuccess)
	{
		// Confirm that path ends in .c3d
		string path = result.filePath;
		if (result.fileName.empty())
		{
			// This isn't currently supporting:
			path = ofFilePath::join(path, generateTakeName());
		}
		else {
			string lower = ofToLower(result.fileName);
			if (lower.size() < 4 || lower.find(".c3d", lower.size() - 4) == string::npos)
			{
				path += ".c3d";
			}
		}
		
		// Save this path
		{
			lock_guard<ofMutex> guard(userSavePathMutex);
			userSavePath = path;
		}
	}

	toggleRecording();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::playbackEvent(ofxRTLSPlaybackArgs& args) {
	
	if (args.bPlay && bRecording) {
		toggleRecording();
	}
}

// --------------------------------------------------------------
float ofxRTLSRecorder::getSavingPercentageComplete()
{
	if (!isSaving()) return 0;
	return float(takeSavingFramesSaved) / float(takeSavingFramesTotal) * 0.95F;
}

// --------------------------------------------------------------
float ofxRTLSRecorder::getRecordingDuration()
{
	if (!bRecording) return 0;
	return float(ofGetElapsedTimeMillis() - thisTakeStartTimeMS) / 1000.0F;
}

// --------------------------------------------------------------
string ofxRTLSRecorder::generateTakePath()
{
	return ofFilePath::getAbsolutePath(ofFilePath::join(takeFolder, generateTakeName()));
}

// --------------------------------------------------------------
string ofxRTLSRecorder::generateTakeName()
{
	return takePrefix + "_" + ofGetTimestampString() + ".c3d";
}

// --------------------------------------------------------------

#endif