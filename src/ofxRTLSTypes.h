#pragma once

#include "ofMain.h"

enum RTLSSystemType {
	RTLS_SYSTEM_TYPE_INVALID = -1,
	RTLS_SYSTEM_TYPE_NULL,
	RTLS_SYSTEM_TYPE_OPENVR,
	RTLS_SYSTEM_TYPE_MOTIVE,
	NUM_RTLS_SYSTEM_TYPES
};

inline string getRTLSSystemTypeDescription(RTLSSystemType systemType)
{
	switch (systemType)
	{
	case RTLS_SYSTEM_TYPE_NULL: return "Null";
	case RTLS_SYSTEM_TYPE_OPENVR: return "OpenVR";
	case RTLS_SYSTEM_TYPE_MOTIVE: return "Motive";
	default: return "Invalid";
	}
}

enum RTLSTrackableType {
	RTLS_TRACKABLE_TYPE_INVALID = -1,
	RTLS_TRACKABLE_TYPE_SAMPLE, // marker
	RTLS_TRACKABLE_TYPE_OBSERVER, // camera, reference
	NUM_RTLS_TRACKABLE_TYPES
};

inline string getRTLSTrackableTypeDescription(RTLSTrackableType trackableType)
{
	switch (trackableType)
	{
	case RTLS_TRACKABLE_TYPE_SAMPLE: return "Sample";
	case RTLS_TRACKABLE_TYPE_OBSERVER: return "Observer";
	default: return "Invalid";
	}
}