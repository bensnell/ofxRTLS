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
	RUI_SHARE_PARAM_WCN("RTLS-R- Record", bRecord);






	bRecord = false;
	RUI_PUSH_TO_CLIENT();

	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRTLSRecorder::paramChanged);


	startThread();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::threadedFunction() {

	RTLSTake* take = NULL;
	vector<RTLSProtocol::TrackableFrame*> frames;
	while (isThreadRunning()) {

		bool bProcessQueue = false;
		{
			// Lock the mutex
			std::unique_lock<std::mutex> lk(mutex);

			// This locks the mutex (if not already locked) in order to check
			// the predicate (whether the queue contains items). If false, the mutex 
			// is unlocked and waits for the condition variable to receive a signal
			// to check again. If true, code execution continues.
			cv.wait(lk, [this] { return flagUnlock || !takeQueue.empty(); });

			if (!flagUnlock) {
				bProcessQueue = true;
			}
			else {
				// Is it still possible for the queue to contain elements here?
			}
		}

		// If there is data waiting, then process it
		while (bProcessQueue) {

			// Get the oldest element (at the front of the queue)
			take = NULL;
			{
				std::lock_guard<std::mutex> lk(mutex);
				// Remove any null elements
				while (takeQueue.front() == NULL) takeQueue.pop();
				// Get the next take to process
				if (!takeQueue.empty()) take = takeQueue.front();
			}
			// Don't continue if there's nothing to process
			if (take == NULL) break;

			// Get the next set of elements to write
			frames.clear(); // should already be clear
			{
				std::lock_guard<std::mutex> lk(mutex);

				// Remove any null elements
				while (takeQueue.front() == NULL) takeQueue.pop();
				// Get the next take to process
				if (!takeQueue.empty()) take = takeQueue.front();
			}



			//// The queue contains elements. Get an element.
			//frame = takeQueue.front();
			//takeQueue.pop();



		}

		//// If an element has been received, process it.
		//if (frame) {

		//	// Save this frame to file



		//	// Delete this data
		//	delete frame;
		//}
	}
}

// --------------------------------------------------------------
void ofxRTLSRecorder::update(RTLSProtocol::TrackableFrame& _frame) {

	// Add the element to the appropriate recording
	{
		std::lock_guard<std::mutex> lk(mutex);
		if (isRecordingPrecise()) {
			RTLSProtocol::TrackableFrame* frame = new RTLSProtocol::TrackableFrame(_frame);
			takeQueue.back()->data.push(frame);
		}
	}
	// Notify the thread that the condition has been met to proceed
	// TODO: should this be done less frequently?
	cv.notify_one();
}

// --------------------------------------------------------------
string ofxRTLSRecorder::getStatus() {

	// TODO: make this safe

	stringstream ss;
	if (!bEnableRecorder) ss << "Recorder is DISABLED";
	if (!isRecording()) ss << "Recording OFF";
	else {
		ss << "Recording to file " << takeQueue.back()->takePath;
		ss << setprecision(2);
		ss << " [" << float(ofGetElapsedTimeMillis() - takeQueue.back()->startTimeMS) / 1000.0 << " sec]";
	}

	return ss.str();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::paramChanged(RemoteUIServerCallBackArg& arg) {
	if (!arg.action == CLIENT_UPDATED_PARAM) return;
	if (!isSetup) return;

	if (arg.paramName.compare("RTLS-R- Record") == 0) {
		if (bRecord && !bRecording) {
			// Begin recording
			// Add a new recording to the queue.
			RTLSTake* take = new RTLSTake();
			take->takePath = ofToDataPath(takeFolder + "/" + takePrefix + "_" + ofGetTimestampString() + ".c3d");
			take->startTimeMS = ofGetElapsedTimeMillis();
			// finalize the start time again with the first measurement
			std::lock_guard<std::mutex> lk(mutex);
			takeQueue.push(take);
		}
		else if (!bRecord && bRecording) {
			// Stop recording
			// Indicate this by adding a NULL to the queue.
			std::lock_guard<std::mutex> lk(mutex);
			takeQueue.push(NULL);
		}
		else {
			// Can't perform this action. Already recording or not recording.
		}
	}
}


// --------------------------------------------------------------

#endif