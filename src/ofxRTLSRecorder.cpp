#include "ofxRTLSRecorder.h"

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





	bFlagStart = false;
	bFlagStop = false;
	RUI_PUSH_TO_CLIENT();

	ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRTLSRecorder::clientDidSomething);


	startThread();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::threadedFunction() {

	RTLSProtocol::TrackableFrame* frame = NULL;
	while (isThreadRunning()) {
		frame = NULL;

		{
			// Lock the mutex
			std::unique_lock<std::mutex> lk(mutex);

			// This locks the mutex (if not already locked) in order to check
			// the predicate (whether the queue contains items). If false, the mutex 
			// is unlocked and waits for the condition variable to receive a signal
			// to check again. If true, code execution continues.
			cv.wait(lk, [this] { return flagUnlock || !dataQueue.empty(); });

			if (!flagUnlock) {
				// The queue contains elements. Get an element.
				frame = dataQueue.front();
				dataQueue.pop();
			}
		}

		// If an element has been received, process it.
		if (frame) {

			// Save this frame to file



			// Delete this data
			delete frame;
		}
	}
}

// --------------------------------------------------------------
void ofxRTLSRecorder::update(RTLSProtocol::TrackableFrame& _frame) {

	// Add the element to the queue
	{
		RTLSProtocol::TrackableFrame* frame = new RTLSProtocol::TrackableFrame(_frame);
		std::lock_guard<std::mutex> lk(mutex);
		dataQueue.push(frame);
	}
	// Notify the thread that the condition has been met to proceed
	cv.notify_one();
}

// --------------------------------------------------------------
string ofxRTLSRecorder::getStatus() {

	stringstream ss;
	if (!bEnableRecorder) ss << "Recorder is DISABLED";
	if (!bRecording) ss << "Recording OFF";
	else {
		ss << "Recording to file " << thisTakePath;
		ss << setprecision(2);
		ss << " [" << float(ofGetElapsedTimeMillis() - thisTakeStartTimeMS) / 1000.0 << " sec]";
	}

	return ss.str();
}

// --------------------------------------------------------------
void ofxRTLSRecorder::paramChanged(RemoteUIServerCallBackArg& arg) {
	if (!arg.action == CLIENT_UPDATED_PARAM) return;

	if (arg.paramName.compare("RTLS-R- Record") == 0) {

	}
}


// --------------------------------------------------------------
