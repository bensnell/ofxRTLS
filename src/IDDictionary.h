#pragma once

#include "ofMain.h"

class IDDictionary {
public:

	IDDictionary();
	~IDDictionary();

	bool setup(string _filepath = "");

	bool isSetup() { return bValid; }

	// Lookup a value mapping
	int lookup(int value);


private:

	// The path at which the dictionary is loaded
	string filepath = "";

	// This is the dictionary, mapping Motive ID (int) to Lamp ID (int)
	vector<int> dict;

	// Is the mapping loaded?
	bool bValid = false;

};