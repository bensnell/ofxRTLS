#pragma once

#include "ofMain.h"
#include "Trackable.pb.h"
using namespace RTLSProtocol;

// The event args output
class ofxRTLSEventArgs : public ofEventArgs {
public:
	TrackableFrame frame;
};
