#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"

class IDDictionary {
public:

	IDDictionary();
	~IDDictionary();

	void setup(string _filepath = "");

	// Lookup a value mapping
	int lookup(int value);


private:

	// The path at which the dictionary is loaded
	string filepath = "id-dictionary.json";

	// This is the dictionary, mapping Motive ID (int) to Lamp ID (int)
	vector<int> dict;

	// Is the mapping enabled?
	bool bEnabled = true;

};