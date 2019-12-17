#include "ofxRTLS.h"

// --------------------------------------------------------------
ofxRTLS::ofxRTLS() {

}

// --------------------------------------------------------------
ofxRTLS::~ofxRTLS() {
	stopThread();
}

// --------------------------------------------------------------
void ofxRTLS::setupParams() {

	RUI_NEW_GROUP("ofxRTLS");

	// Setup RUI Params

#ifdef RTLS_VIVE
	// Are there any params?

#endif
#ifdef RTLS_MOTIVE
	motive.setupParams();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::setup() {
	// Add listeners for new data
#ifdef RTLS_VIVE
	ofAddListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);
#endif
#ifdef RTLS_MOTIVE
	ofAddListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif

	startThread();
}

// --------------------------------------------------------------
void ofxRTLS::start() {
	// Start communications with the trackers
#ifdef RTLS_VIVE
	vive.connect();
#endif
#ifdef RTLS_MOTIVE
	motive.start();
#endif

}

// --------------------------------------------------------------
void ofxRTLS::stop() {
	// Stop communication with the trackers
#ifdef RTLS_VIVE
	vive.disconnect();
#endif
#ifdef RTLS_MOTIVE
	motive.stop();
#endif
}

// --------------------------------------------------------------
#ifdef RTLS_MOTIVE

void ofxRTLS::motiveDataReceived(MotiveEventArgs& args) {

	lastReceive = ofGetElapsedTimeMillis();

	// Send each identified point
	RTLSEventArgs outArgs;
	outArgs.frame.set_frame_id(1);
	for (int i = 0; i < args.markers.size(); i++) {

		Trackable* trackable = outArgs.frame.add_trackables();
		char byte_array[16];
		((uint64_t*)byte_array)[0] = args.markers[i].cuid.HighBits();
		((uint64_t*)byte_array)[1] = args.markers[i].cuid.LowBits();
		trackable->set_cuid(byte_array, 16);
		Trackable::Position* position = trackable->mutable_position();
		position->set_x(args.markers[i].position.x);
		position->set_y(args.markers[i].position.y);
		position->set_z(args.markers[i].position.z);
	}

	ofNotifyEvent(newFrameReceived, outArgs);

	// Save this frame
	lastFrame = outArgs.frame;
}

#endif

// --------------------------------------------------------------
#ifdef RTLS_VIVE

void ofxRTLS::openvrDataReceived(ofxOpenVRTrackerEventArgs& args) {

	lastReceive = ofGetElapsedTimeMillis();
	if (!bOscEnabled) return;

	// Send each identified point
	ofxOscMessage m;

	for (int i = 0; i < (*args.devices->getTrackers()).size(); i++) {

		Device* tkr = (*args.devices->getTrackers())[i];
		if (tkr->isActive()) {

			m.clear();
			m.setAddress(messageAddress);

			m.addStringArg(tkr->serialNumber);
			m.addFloatArg(tkr->position.x);
			m.addFloatArg(tkr->position.y);
			m.addFloatArg(tkr->position.z);
			m.addFloatArg(tkr->quaternion.w);
			m.addFloatArg(tkr->quaternion.x);
			m.addFloatArg(tkr->quaternion.y);
			m.addFloatArg(tkr->quaternion.z);

			// should this also send whether it's new data or old data? (keep alive)
			// should this send time?

			sender.sendMessage(m, false);

			// Log this data
			lastSend = ofGetElapsedTimeMillis();
		}
	}

	// Save this message
	lastMessage = m;
}

#endif

// --------------------------------------------------------------
void ofxRTLS::threadedFunction() {

	while (isThreadRunning()) {

		// Determine whether we are actively receiving any messages in the last
		// stopGap milliseconds
		bReceivingData = ofGetElapsedTimeMillis() - lastReceive < stopGap;

	}
}

// --------------------------------------------------------------
void ofxRTLS::exit() {

#ifdef RTLS_VIVE
	// Remove listener for new device data
	ofRemoveListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	vive.exit();
#endif
#ifdef RTLS_MOTIVE
	ofRemoveListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif
}

// --------------------------------------------------------------
bool ofxRTLS::isConnected() {

#ifdef RTLS_VIVE
	return vive.isConnected();
#endif
#ifdef RTLS_MOTIVE
	return motive.isConnected();
#endif

	return false;
}

// --------------------------------------------------------------
bool ofxRTLS::isReceivingData() {
	return bReceivingData; // no need to lock for a bool
}

