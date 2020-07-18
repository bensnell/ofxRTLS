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
				if (takeQueue.front()->bFlagSave) {
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

			// Proceed with saving the take
			// TODO



			// Delete the take
			take->clear();
			delete take;
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSRecorder::update(RTLSProtocol::TrackableFrame& _frame) {
	if (!isSetup) return;
	if (!bEnableRecorder) return;
	if (!bRecording) return;

	// Add the element to the appropriate recording
	std::lock_guard<std::mutex> lk(mutex);

	// Only add the frame if there is a take in the queue (this shouldn't need to be checked, but
	// let's do it for safety).
	if (takeQueue.empty()) return;

	// If this is the first frame, set the start time
	if (takeQueue.back()->startTimeMS == 0) takeQueue.back()->startTimeMS = ofGetElapsedTimeMillis();

	// TODO:
	// copy the frame data to a c3d container and push it onto the queue




	// Add data to the last take
	//takeQueue.back()->data.push(_frame);
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
			thisTakePath = ofToDataPath(takeFolder + "/" + takePrefix + "_" + ofGetTimestampString() + ".c3d");
			thisTakeStartTimeMS = ofGetElapsedTimeMillis();
			take->name = thisTakePath;

			std::lock_guard<std::mutex> lk(mutex);
			takeQueue.push(take);
			bRecording = true;
		}
		else if (!bShouldRecord && bRecording) {
			// Stop recording
			bRecording = false;

			// Flag that we should save this take
			std::lock_guard<std::mutex> lk(mutex);
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

#endif