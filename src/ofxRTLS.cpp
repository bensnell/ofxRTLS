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
	
#ifdef RTLS_NULL
	// Setup params
	nsys.setup();
	// Add listener for new data
	ofAddListener(nsys.newDataReceived, this, &ofxRTLS::nsysDataReceived);
	
#ifdef RTLS_POSTPROCESS
	nsysPostM.setup("NullSysMarkers", "NM");
#endif
#endif

#ifdef RTLS_OPENVR
	// Add listeners for new data
	ofAddListener(openvr.newDataReceived, this, &ofxRTLS::openvrDataReceived);
	
#ifdef RTLS_POSTPROCESS
	// Setup the postprocessor
	openvrPostM.setup("OpenVRMarkers", "OM");
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
	motivePostM.setup("MotiveMarkers", "MM", "id-dictionary.json", 
		"age,axes,kalman,easing,add-rate,continuity,easing");
	motivePostR.setup("MotiveRef", "MR", "", "axes");
#endif
#endif

	// Set target frame rate
	dataFPS = ofGetTargetFrameRate();

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

	lastReceive = ofGetElapsedTimeMillis();
	mutex.lock();
	dataTimestamps.push(lastReceive);
	mutex.unlock();

	// ==============================================
	// Fake Data
	// Send every frame
	// ==============================================

	ofJson js;
	js["s"] = 0; // system = motive
	js["t"] = 0; // type = marker

	ofxRTLSEventArgs outArgs;
	outArgs.frame.set_context(js.dump());
	outArgs.frame.set_frame_id(nsysFrameID);
	outArgs.frame.set_timestamp(ofGetElapsedTimeMillis());

	for (int i = 0; i < args.markers.size(); i++) {

		Trackable* trackable = outArgs.frame.add_trackables();
		trackable->set_id(i);
		Trackable::Position* position = trackable->mutable_position();
		position->set_x(args.markers[i].x);
		position->set_y(args.markers[i].y);
		position->set_z(args.markers[i].z);
	}

#ifdef RTLS_POSTPROCESS
	// Post-process the data, then send it out when ready
	nsysPostM.processAndSend(outArgs, newFrameReceived);
#else
	// Send out data immediately
	ofNotifyEvent(newFrameReceived, outArgs);
#endif

	nsysFrameID++;
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

	ofJson js;
	js["s"] = 1; // system = openvr
	js["t"] = 0; // type = marker

	ofxRTLSEventArgs outArgs;
	outArgs.frame.set_context(js.dump());
	outArgs.frame.set_frame_id(openvrFrameID);
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

#ifdef RTLS_POSTPROCESS
	// Post-process the data, then send it out when ready
	openvrPostM.processAndSend(outArgs, newFrameReceived);
#else
	// Send out data immediately
	ofNotifyEvent(newFrameReceived, outArgs);
#endif

	openvrFrameID++;
}

#endif

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

	ofJson js;
	js["s"] = 2; // system = motive
	js["t"] = 0; // type = marker

	// Send each identified marker
	ofxRTLSEventArgs mOutArgs;
	mOutArgs.frame.set_context(js.dump());
	mOutArgs.frame.set_frame_id(motiveFrameID);
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

#ifdef RTLS_POSTPROCESS
	// Post-process the data, then send it out when ready
	motivePostM.processAndSend(mOutArgs, newFrameReceived);
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

		js.clear();
		js["s"] = 2; // system = motive
		js["t"] = 1; // type = reference
		js["m"] = int(args.maybeNeedsCalibration);

		ofxRTLSEventArgs cOutArgs;
		cOutArgs.frame.set_context(js.dump());
		cOutArgs.frame.set_frame_id(motiveFrameID);
		cOutArgs.frame.set_timestamp(ofGetElapsedTimeMillis());

		// Add all cameras (after postprocessing)
		for (int i = 0; i < args.cameras.size(); i++) {

			js.clear();
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

#ifdef RTLS_POSTPROCESS
		// Post-process the data, then send it out when ready
		motivePostR.processAndSend(cOutArgs, newFrameReceived);
#else
		// Send out camera data immediately
		ofNotifyEvent(newFrameReceived, cOutArgs);
#endif
	}
	
	motiveFrameID++;
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

// --------------------------------------------------------------

// --------------------------------------------------------------

// --------------------------------------------------------------
