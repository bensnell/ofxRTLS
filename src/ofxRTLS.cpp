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

	// Setup general RTLS params
	//RUI_NEW_GROUP("ofxRTLS");
	
#ifdef RTLS_OPENVR
	
	// Add listeners for new data
	ofAddListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	
#ifdef RTLS_ENABLE_POSTPROCESS
	// Setup the postprocessor
	tPost.setup("ViveTrackers", "T");
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

#ifdef RTLS_ENABLE_POSTPROCESS
	// Setup the postprocessors
	mPost.setup("MotiveMarkers", "M", "id-dictionary.json", 
		"age,axes,kalman,easing,add-rate,continuity,easing");
	cPost.setup("MotiveCameras", "C", "", "axes");
#endif

#endif

	// Set target frame rate
	dataFPS = ofGetTargetFrameRate();

	startThread();
}

// --------------------------------------------------------------
void ofxRTLS::start() {
	// Start communications with the trackers
#ifdef RTLS_OPENVR
	vive.connect();
#endif
#ifdef RTLS_MOTIVE
	motive.start();
#endif
}

// --------------------------------------------------------------
void ofxRTLS::stop() {
	// Stop communication with the trackers
#ifdef RTLS_OPENVR
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
	mutex.lock();
	dataTimestamps.push(lastReceive);
	mutex.unlock();

	// ==============================================
	// Marker Trackables
	// Send every frame
	// ==============================================

	// Send each identified marker
	ofxRTLSEventArgs mOutArgs;
	mOutArgs.frame.set_context("m"); // 'm' for marker
	mOutArgs.frame.set_frame_id(frameID);
	mOutArgs.frame.set_timestamp(ofGetElapsedTimeMillis());

	for (int i = 0; i < args.markers.size(); i++) {

		Trackable* trackable = mOutArgs.frame.add_trackables();
		char byte_array[16];
		((uint64_t*)byte_array)[0] = args.markers[i].cuid.LowBits();
		((uint64_t*)byte_array)[1] = args.markers[i].cuid.HighBits();
		trackable->set_cuid(byte_array, 16);
		// Set the ID (-1 for passive, >=0 for active)
		trackable->set_id(getActiveMarkerID(args.markers[i].cuid));
		Trackable::Position* position = trackable->mutable_position();
		position->set_x(args.markers[i].position.x);
		position->set_y(args.markers[i].position.y);
		position->set_z(args.markers[i].position.z);
	}

#ifdef RTLS_ENABLE_POSTPROCESS
	// Post-process the data, then send it out when ready
	mPost.processAndSend(mOutArgs, newFrameReceived);
#else
	// Send out the data immediately
	ofNotifyEvent(newFrameReceived, mOutArgs);
#endif

	// ==============================================
	// Camera Trackables
	// Send with specified period.
	// ==============================================

	uint64_t thisTime = ofGetElapsedTimeMillis();
	if (bSendCameraData && ((lastSendTime == 0) || (thisTime - lastSendTime >= cameraDataFrequency*1000.0))) {
		lastSendTime = thisTime;

		ofxRTLSEventArgs cOutArgs;
		cOutArgs.frame.set_context("c"); // 'c' for camera
		cOutArgs.frame.set_frame_id(frameID);
		cOutArgs.frame.set_timestamp(ofGetElapsedTimeMillis());

		// Add all cameras (after postprocessing)
		for (int i = 0; i < args.cameras.size(); i++) {

			Trackable* trackable = cOutArgs.frame.add_trackables();
			trackable->set_id(args.cameras[i].ID);
			trackable->set_cuid(ofToString(args.cameras[i].serial));
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

#ifdef RTLS_ENABLE_POSTPROCESS
		// Post-process the data, then send it out when ready
		cPost.processAndSend(cOutArgs, newFrameReceived);
#else
		// Send out camera data immediately
		ofNotifyEvent(newFrameReceived, cOutArgs);
#endif
	}
	
	frameID++;
}

#endif

// --------------------------------------------------------------
#ifdef RTLS_OPENVR

void ofxRTLS::openvrDataReceived(ofxOpenVRTrackerEventArgs& args) {

	lastReceive = ofGetElapsedTimeMillis();
	mutex.lock();
	dataTimestamps.push(lastReceive);
	mutex.unlock();

	// ==============================================
	// Generic Tracker Trackables
	// Send every frame
	// ==============================================

	ofxRTLSEventArgs outArgs;
	outArgs.frame.set_frame_id(frameID);
	outArgs.frame.set_timestamp(ofGetElapsedTimeMillis());

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

#ifdef RTLS_ENABLE_POSTPROCESS
	// Post-process the data, then send it out when ready
	tPost.processAndSend(outArgs, newFrameReceived);
#else
	// Send out data immediately
	ofNotifyEvent(newFrameReceived, outArgs);
#endif

	frameID++;
}

#endif

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
		dataFPS = float(dataTimestamps.size());
		mutex.unlock();
		
		sleep(16);
	}
}

// --------------------------------------------------------------
void ofxRTLS::exit() {

#ifdef RTLS_OPENVR
#ifdef RTLS_ENABLE_POSTPROCESS
	tPost.exit();
#endif
	// Remove listener for new device data
	ofRemoveListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	vive.exit();
#endif
#ifdef RTLS_MOTIVE
#ifdef RTLS_ENABLE_POSTPROCESS
	mPost.exit();
	cPost.exit();
#endif
	ofRemoveListener(motive.newDataReceived, this, &ofxRTLS::motiveDataReceived);
#endif
}

// --------------------------------------------------------------
bool ofxRTLS::isConnected() {

#ifdef RTLS_OPENVR
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

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------
