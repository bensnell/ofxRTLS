#pragma once

#include "ofMain.h"

class ofxRTLSConfigManager
{
public:

	static ofxRTLSConfigManager* one();

	// Load the rtls configuration
	bool load(string config_path = "");

	// Check if options are enabled
	bool motive() const { return motive_; }
	bool openvr() const { return openvr_; }
	bool null() const { return null_; }
	bool postprocess() const { return postprocess_; }
	bool player() const { return player_; }

	// Optional project metadata that may be supplied in the rtls config file:
	bool project_metadata_exists() { return project_metadata_exists_; }
	string project_name() const { return project_name_; }
	string project_version() const { return project_version_; }
	string project_commit() const { return project_commit_; }
	string project_repo() const { return project_repo_; }
	
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

	bool project_metadata_exists_ = false;
	string project_name_ = "";
	string project_version_ = "";
	string project_commit_ = "";
	string project_repo_ = "";
	
};

