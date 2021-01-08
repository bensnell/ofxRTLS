#pragma once

#include "ofMain.h"

class ofxRTLSConfigManager
{
public:

	static ofxRTLSConfigManager* one();

	bool load(string config_path = "");

	bool motive() const { return motive_; }
	bool openvr() const { return openvr_; }
	bool null() const { return null_; }
	bool postprocess() const { return postprocess_; }
	bool player() const { return player_; }
	
private:

	static ofxRTLSConfigManager* instance_;

	ofxRTLSConfigManager() {};
	~ofxRTLSConfigManager() {};

	string config_path_ = "configs/rtls-config.json";
	bool loaded_ = false;
	
	bool null_ = false;
	bool openvr_ = false;
	bool motive_ = false;
	bool postprocess_ = false;
	bool player_ = false;
};

