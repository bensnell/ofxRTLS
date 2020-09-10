#pragma once

#include "ofMain.h"

enum RTLSSystemType {
	RTLS_SYSTEM_TYPE_INVALID = -1,
	RTLS_SYSTEM_TYPE_NULL,
	RTLS_SYSTEM_TYPE_OPENVR,
	RTLS_SYSTEM_TYPE_MOTIVE,
	NUM_RTLS_SYSTEM_TYPES
};

enum RTLSTrackableType {
	RTLS_TRACKABLE_TYPE_INVALID = -1,
	RTLS_TRACKABLE_TYPE_SAMPLE, // marker
	RTLS_TRACKABLE_TYPE_OBSERVER, // camera, reference
	NUM_RTLS_TRACKABLE_TYPES
};
