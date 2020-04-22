#include "ofxRTLS.h"

// --------------------------------------------------------------
ofxRTLS::ofxRTLS() {

}

// --------------------------------------------------------------
ofxRTLS::~ofxRTLS() {
	stopThread();
}

// --------------------------------------------------------------
void ofxRTLS::setup() {

	// Setup general RTLS params
	//RUI_NEW_GROUP("ofxRTLS");

	
#ifdef RTLS_OPENVR
	
	// Add listeners for new data
	ofAddListener(vive.newDataReceived, this, &ofxRTLS::openvrDataReceived);

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

#endif

#ifdef RTLS_ENABLE_POSTPROCESS
	// Setup postprocessing params
	RUI_NEW_GROUP("ofxRTLS Postprocessing");
	RUI_SHARE_PARAM_WCN("RTLSPost- Map IDs", bMapIDs);
	RUI_SHARE_PARAM_WCN("RTLSPost- Remove Invalid IDs", bRemoveInvalidIDs);
	RUI_SHARE_PARAM_WCN("RTLSPost- Apply Filters", bApplyFilters);

	// Setup processing helpers
	dict.setup("id-dictionary.json");
	filters.setup("RTLS", "kalman,easing,add-rate,continuity,easing");
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
	mtx.lock();
	dataTimestamps.push(lastReceive);
	mtx.unlock();

	// ==============================================
	// Marker Trackables
	// Send every frame
	// ==============================================

	// Send each identified marker
	RTLSEventArgs mOutArgs;
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

	// Post-process the data
	postprocess(mOutArgs.frame);

	// Send out marker data
	ofNotifyEvent(newFrameReceived, mOutArgs);

	// ==============================================
	// Camera Trackables
	// Send with specified period.
	// ==============================================

	uint64_t thisTime = ofGetElapsedTimeMillis();
	if (bSendCameraData && ((lastSendTime == 0) || (thisTime - lastSendTime >= cameraDataFrequency*1000.0))) {
		lastSendTime = thisTime;

		RTLSEventArgs cOutArgs;
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

		// Send out camera data
		ofNotifyEvent(newFrameReceived, cOutArgs);
	}
	
	frameID++;
}

#endif

// --------------------------------------------------------------
#ifdef RTLS_OPENVR

void ofxRTLS::openvrDataReceived(ofxOpenVRTrackerEventArgs& args) {

	lastReceive = ofGetElapsedTimeMillis();
	mtx.lock();
	dataTimestamps.push(lastReceive);
	mtx.unlock();

	// Send each identified point
	RTLSEventArgs outArgs;
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

	// Post-process the data
	postprocess(outArgs.frame);

	ofNotifyEvent(newFrameReceived, outArgs);

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
		mtx.lock();
		while (!dataTimestamps.empty() && dataTimestamps.front() < (thisTime-1000)) {
			dataTimestamps.pop();
		}
		dataFPS = float(dataTimestamps.size());
		mtx.unlock();
	}
}

// --------------------------------------------------------------
void ofxRTLS::exit() {

#ifdef RTLS_OPENVR
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
void ofxRTLS::postprocess(RTLSProtocol::TrackableFrame& frame) {
#ifdef RTLS_ENABLE_POSTPROCESS

	if (bMapIDs) {	// Map IDs if they are valid
		for (int i = 0; i < frame.trackables_size(); i++) {
			int ID = frame.trackables(i).id();
			if (ID < 0) continue;
			frame.mutable_trackables(i)->set_id(dict.lookup(ID));
		}
	}

	if (bRemoveInvalidIDs) { // Remove IDs that are invalid (< 0)
		int i = 0;
		while (i < frame.trackables_size()) {

			// Check if this trackable's ID is invalid
			if (frame.trackables(i).id() < 0) {
				// Arbitrary elements cannot be deleted. Swap this element with the last
				// and remove the last element.
				frame.mutable_trackables()->SwapElements(i, frame.trackables_size() - 1);
				frame.mutable_trackables()->RemoveLast();
			}
			else {
				i++;
			}
		}
	}

	// For all that remain in the filter, set their new coordinates and export them
	if (bApplyFilters) {
		// Input the new data
		for (int i = 0; i < frame.trackables_size(); i++) {
			// Add new data to the filter 
			glm::vec3 position = glm::vec3(
				frame.trackables(i).position().x(),
				frame.trackables(i).position().y(),
				frame.trackables(i).position().z());
			auto* filter = filters.getFilter(getFilterKey(frame.trackables(i)));
			filter->process(position);
		}

		// Process any remaining filters that haven't seen data
		filters.processRemaining();

		// Delete any data that is invalid.
		// Also save IDs for all data that is valid.
		set<string> existingDataIDs;
		int i = 0;
		while (i < frame.trackables_size()) {
			// Check if this trackable's data is invalid.
			ofxFilter* filter = filters.getFilter(getFilterKey(frame.trackables(i)));
			if (!filter->isDataValid()) {
				// If not, delete it
				frame.mutable_trackables()->SwapElements(i, frame.trackables_size() - 1);
				frame.mutable_trackables()->RemoveLast();
			}
			else {
				// Save that this ID has valid data
				existingDataIDs.insert(getFilterKey(frame.trackables(i)));

				// Set this new processed data
				glm::vec3 data = filter->getPosition();
				frame.mutable_trackables(i)->mutable_position()->set_x(data.x);
				frame.mutable_trackables(i)->mutable_position()->set_y(data.y);
				frame.mutable_trackables(i)->mutable_position()->set_z(data.z);

				i++;
			}
		}

		// Add any data that isn't present
		for (auto& it : filters.getFilters()) {
			// Check if this is a new ID and if it has valid data.
			if (existingDataIDs.find(it.first) == existingDataIDs.end() && it.second->isDataValid()) {
				// If so, add a trackable with this ID				
				Trackable* trackable = frame.add_trackables();
				if (it.first.size() == 16) {	// cuid
					trackable->set_id(-1);
					trackable->set_cuid(it.first);
				}
				else if (!it.first.empty()) {	// id
					trackable->set_id(ofToInt(it.first));
				}
				Trackable::Position* position = trackable->mutable_position();
				glm::vec3 data = it.second->getPosition();
				position->set_x(data.x);
				position->set_y(data.y);
				position->set_z(data.z);
			}
		}

		// Delete any filters that haven't been used recently
		if (ofGetElapsedTimeMillis() - lastFilterCullingTime > filterCullingPeriod) {
			lastFilterCullingTime = ofGetElapsedTimeMillis();
			filters.removeUnused();
		}
	}
#endif
}

// --------------------------------------------------------------
#ifdef RTLS_ENABLE_POSTPROCESS
string ofxRTLS::getFilterKey(const Trackable& t) {

	// Use the ID as the key, if valid (>= 0)
	if (t.id() >= 0) return ofToString(t.id());

	// Otherwise, use the cuid
	if (!t.cuid().empty()) return t.cuid();

	// Otherwise, return an empty string
	return "";
}
#endif
// --------------------------------------------------------------
