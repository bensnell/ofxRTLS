#include "ofxRTLSConfigManager.h"

ofxRTLSConfigManager* ofxRTLSConfigManager::instance_ = NULL;

// ----------------------------------------------------------------------------
ofxRTLSConfigManager* ofxRTLSConfigManager::one() {
    if (!instance_) {
		instance_ = new ofxRTLSConfigManager();
    }
    return instance_;
}

// ----------------------------------------------------------------------------
bool ofxRTLSConfigManager::load(string config_path)
{
	// Don't re-load the configuration file
	if (loaded_) return true;

	// Set a new path, if applicable
    if (!config_path.empty() && ofFile::doesFileExist(config_path, true))
		config_path_ = config_path;

	// Make sure the file exists
    if (!ofFile::doesFileExist(config_path_))
    {
		ofLogWarning("ofxRTLSConfigManager") << "Config file does not exist. Default settings loaded.";
		return false;
    }

	// Attempt to load the file
	ofFile file(ofToDataPath(config_path_));
	if (!file.exists()) {
		ofLogError("ofxRTLSConfigManager") << "Config file does not exist. Default settings loaded.";
		return false;
	}
	ofJson js;
	file >> js;
	if (js.find("systems") == js.end()) {
		ofLogWarning("ofxRTLSConfigManager") << "No tracking systems listed. None enabled.";
	} else
	{
		if (!js.find("systems")->is_array())
		{
			ofLogWarning("ofxRTLSConfigManager") << "Tracking systems must be supplied in a list. None enabled.";
		} else
		{
			auto n_invalid_systems = 0;
			for (auto i = 0; i < js.find("systems")->size(); i++)
			{
				auto system = js.find("systems")->at(i).get<string>();
				system = ofToLower(system);
				if (system == "null")
					null_ = true;
				else if (system == "openvr")
					openvr_ = true;
				else if (system == "motive")
					motive_ = true;
				else
					n_invalid_systems++;
			}
			if (n_invalid_systems == js.find("systems")->size())
			{
				ofLogError("ofxRTLSConfigManager") << "No listed tracking systems are valid. None enabled.";
			}
		}
	}
	
	if (js.find("postprocess") != js.end())
		postprocess_ = js.find("postprocess")->get<bool>();
	
	if (js.find("player") != js.end())
		player_ = js.find("player")->get<bool>();

	// Try to get the optional parameters
	if (js.find("project_metadata") != js.end())
	{
		if (js["project_metadata"].find("name") != js["project_metadata"].end())
			project_name_ = js["project_metadata"]["name"].get<string>();
		
		if (js["project_metadata"].find("version") != js["project_metadata"].end())
			project_version_ = js["project_metadata"]["version"].get<string>();

		if (js["project_metadata"].find("commit") != js["project_metadata"].end())
			project_commit_ = js["project_metadata"]["commit"].get<string>();

		if (js["project_metadata"].find("repo") != js["project_metadata"].end())
			project_repo_ = js["project_metadata"]["repo"].get<string>();

		project_metadata_exists_ = !project_name_.empty() || !project_version_.empty()
			|| !project_commit_.empty() || !project_repo_.empty();
	}
	
	ofLogNotice("ofxRTLSConfigManager") << "Configuration file loaded from \"" << config_path_ << "\"";

	loaded_ = true;
	return true;
}

// ----------------------------------------------------------------------------
string ofxRTLSConfigManager::project_metadata()
{
	stringstream ss;
	if (project_metadata_exists())
	{
		ss << "ofxRTLS configured";
		if (!project_name().empty()) ss << " for project " << project_name() << "";
		if (!project_version().empty()) ss << " with version " << project_version() << "";
		if (!project_commit().empty()) ss << " at commit " << project_commit() << "";
		if (!project_repo().empty()) ss << " using repository " << project_repo() << "";
		ss << ".";
	} else
	{
		ss << "ofxRTLS not configured for specific project.";
	}
	return ss.str();
}

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------