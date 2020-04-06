#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"

class IDDictionary {
public:

	IDDictionary();
	~IDDictionary();

	void setup(string _filepath);



private:

	string filepath = "id-dictionary.json";

};