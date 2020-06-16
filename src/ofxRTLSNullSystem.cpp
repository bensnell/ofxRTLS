#include "ofxRTLSNullSystem.h"

// --------------------------------------------------------------
ofxRTLSNullSystem::ofxRTLSNullSystem() {

}

// --------------------------------------------------------------
ofxRTLSNullSystem::~ofxRTLSNullSystem() {
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::setup() {

	RUI_NEW_GROUP("ofxRTLS Null System");
	RUI_SHARE_PARAM_WCN("NuS- Send Fake Data", bSendFakeData);
	RUI_SHARE_ENUM_PARAM_WCN("NuS- Data Mode", mode, ELASTIC, BROWNIAN, { "Elastic", "Brownian" });
	RUI_SHARE_PARAM_WCN("NuS- Frame Rate", fps, 0, 1000);
	RUI_SHARE_PARAM_WCN("NuS- nPoints", nPoints, 0, 1000);
	RUI_SHARE_PARAM_WCN("NuS- Space Lo Bound", loBound, -10000, 10000);
	RUI_SHARE_PARAM_WCN("NuS- Space Hi Bound", hiBound, -10000, 10000);
	RUI_SHARE_PARAM_WCN("NuS- Position Speed", positionSpeed, -1000, 1000);
	RUI_SHARE_PARAM_WCN("NuS- Position Noise", positionNoise, -100, 100);
	RUI_SHARE_PARAM_WCN("NuS- Elastic Force", elasticForce, 0, 10);
	RUI_SHARE_PARAM_WCN("NuS- Elastic Winds", elasticWinds, 0, 1);
	RUI_SHARE_PARAM_WCN("NuS- Target Presence Density", targetPresenceDensity, 0, 1);
	RUI_SHARE_PARAM_WCN("NuS- Presence Return Rapidness", presenceReturnRapidness, 0, 10);
	RUI_SHARE_PARAM_WCN("NuS- Set ID", bSetID);
	RUI_SHARE_PARAM_WCN("NuS- Set CUID", bSetCUID);
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::start() {
	startThread();
	bConnected = true;
}

// --------------------------------------------------------------
bool ofxRTLSNullSystem::isConnected() {
	return bConnected;
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::stop() {
	bConnected = false;
	waitForThread(true);
}

// --------------------------------------------------------------
void ofxRTLSNullSystem::threadedFunction() {

	while (isThreadRunning()) {

		if (bSendFakeData) {
			uint64_t thisTime = ofGetElapsedTimeMillis();
			if (float(thisTime - lastTime) >= (1000.0 / fps)) {
				lastTime = thisTime;

				// Adjust the number of trackables
				int lastSize = trackables.size();
				int thisSize = nPoints;
				if (lastSize != thisSize) {
					
					trackables.resize(thisSize);

					// Initialize any new trackables
					for (int i = lastSize; i < thisSize; i++) {
						// Set the position
						trackables[i].setKnownPosition(glm::vec3(
							ofRandom(loBound, hiBound), 
							ofRandom(loBound, hiBound), 
							ofRandom(loBound, hiBound)));
						// Set a velocity
						trackables[i].velocity = glm::vec3(
							ofRandom(-positionSpeed, positionSpeed),
							ofRandom(-positionSpeed, positionSpeed),
							ofRandom(-positionSpeed, positionSpeed));
						trackables[i].velocity /= fps; // normalize
						trackables[i].maxSpeedFraction = ofRandom(1);
						// Set the ID
						if (bSetID) trackables[i].setId(i + 1);
						// Set the CUID
						if (bSetCUID) {
							trackables[i].setCuid(ofToString(cuidCounter));
							cuidCounter++;
						}
					}
				}
				
				// Calculate the probability that a trackable becomes present or absent
				float rapidnessMultiplier = 1.0 / (1.0 + fps * presenceReturnRapidness);
				float probBecomesPresent = targetPresenceDensity / max(targetPresenceDensity, float(1.0 - targetPresenceDensity)) * rapidnessMultiplier;
				float probBecomesAbsent = (1.0 - targetPresenceDensity) / max(targetPresenceDensity, float(1.0 - targetPresenceDensity)) * rapidnessMultiplier;

				// Update all trackables
				for (int i = 0; i < trackables.size(); i++) {
					auto& t = trackables[i];

					// Update whether this trackable is occluded or present.
					float prob = ofRandom(1);
					if (t.bPresent && prob < probBecomesAbsent) {
						// goes absent
						t.bPresent = false;
						t.clearCuid();
					}
					else if (!t.bPresent && prob < probBecomesPresent) {
						// goes present
						t.bPresent = true;
					}

					// Update the position with brownian motion
					switch (mode) {
					case ELASTIC: {

						// Update forces on the particle based on where it is
						float margin = 0.1 * (hiBound - loBound);
						glm::vec3 p = t.getKnownPosition();
						t.force = glm::vec3(
							float(p.x < (loBound + margin)) * ((loBound + margin) - p.x) + float(p.x > (hiBound - margin)) * ((hiBound - margin) - p.x),
							float(p.y < (loBound + margin)) * ((loBound + margin) - p.y) + float(p.y > (hiBound - margin)) * ((hiBound - margin) - p.y),
							float(p.z < (loBound + margin)) * ((loBound + margin) - p.z) + float(p.z > (hiBound - margin)) * ((hiBound - margin) - p.z));
						// The force will be the amount of change of velocity (acceleration) per frame
						t.force = t.force / margin * elasticForce / fps;

						// Add some random "winds" to the forces
						glm::vec3 noise = glm::vec3(
							ofNoise(p.x, i * 31.73, ofGetElapsedTimef()),
							ofNoise(p.y, i * 31.73, ofGetElapsedTimef() + 71.217),
							ofNoise(p.z, i * 31.73, ofGetElapsedTimef() + 191.71)
						) * 2.0 - glm::vec3(1, 1, 1);
						t.force += noise * elasticWinds / fps;

						// Update the velocity
						t.velocity += t.force;
						// Clamp the speed
						if (glm::l2Norm(t.velocity) > positionSpeed / fps * t.maxSpeedFraction) {
							t.velocity = glm::normalize(t.velocity) * positionSpeed / fps * t.maxSpeedFraction;
						}

						// Update the position
						t.setKnownPosition(t.getKnownPosition() + t.velocity);

					}; break;
					case BROWNIAN: default: {

						glm::vec3 lastPos = t.getKnownPosition();
						float speed = positionSpeed / fps;
						glm::vec3 changePos = glm::vec3(ofRandom(-speed, speed), ofRandom(-speed, speed), ofRandom(-speed, speed));
						glm::vec3 newPos = lastPos + changePos;
						t.setKnownPosition(newPos);

					}; break;
					}

					// Set the noise associated with the position
					t.setPositionNoise(glm::vec3(
						ofRandom(-positionNoise, positionNoise), 
						ofRandom(-positionNoise, positionNoise),
						ofRandom(-positionNoise, positionNoise)));

					// Set or clear the ID
					if (bSetID && !t.hasId()) t.setId(i + 1);
					else if (!bSetID && t.hasId()) t.clearId();

					// Set or clear the CUID
					if (bSetCUID && !t.hasCuid()) {
						t.setCuid(ofToString(cuidCounter));
						cuidCounter++;
					}
					else if (!bSetCUID && t.hasCuid()) {
						t.clearCuid();
					}
				}

				// Copy the data to event args and sent it
				NullSystemEventArgs args;
				for (auto& t : trackables) {
					if (!t.bPresent) continue;
					args.trackables.push_back(NullSystemTrackable(t));
				}
				ofNotifyEvent(newDataReceived, args);

				// Sleep for a bit
				sleep(int(floor(1000.0 / fps * 0.99)));
			}
			else {
				sleep(1);
			}
		}
		else {
			sleep(1000);
		}
	}
}

// --------------------------------------------------------------

// --------------------------------------------------------------
