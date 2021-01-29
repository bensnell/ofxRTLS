#pragma once

// Easy access macros
#define OFX_RTLS_NULL_ENABLED() ( ofxRTLSConfigManager::one()->null() )
#define OFX_RTLS_OPENVR_ENABLED() ( ofxRTLSConfigManager::one()->openvr() )
#define OFX_RTLS_MOTIVE_ENABLED() ( ofxRTLSConfigManager::one()->motive() )
#define OFX_RTLS_POSTPROCESS_ENABLED() ( ofxRTLSConfigManager::one()->postprocess() )
#define OFX_RTLS_PLAYER_ENABLED() ( ofxRTLSConfigManager::one()->player() )
#define OFX_RTLS_CONFIG_MANAGER() ( ofxRTLSConfigManager::one() )

#define RTLS_NULL			OFX_RTLS_NULL_ENABLED
#define RTLS_OPENVR			OFX_RTLS_OPENVR_ENABLED
#define RTLS_MOTIVE			OFX_RTLS_MOTIVE_ENABLED
#define RTLS_POSTPROCESS	OFX_RTLS_POSTPROCESS_ENABLED
#define RTLS_PLAYER			OFX_RTLS_PLAYER_ENABLED
#define RTLS_CONFIG			OFX_RTLS_CONFIG_MANAGER