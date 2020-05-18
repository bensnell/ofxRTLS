#pragma once

#include "ofMain.h"
#include "ofxRTLS.h"
#include "ofxOsc.h"
#include "ofxRemoteUIServer.h"

class OSCThread : public ofThread {
	public:
		void setup();
		void RTLSFrameReceived(ofxRTLSEventArgs& args);

		void exit();

		/// \brief Draw the status of OSC
		void drawStatus(int x, int y);

		bool isOscEnabled();
		bool isOscSending();

		void setOscEnabled(bool _bOscEnabled);

		/// \brief Get OSC information
		string getOscHostAddress();
		int getOscPort();
		string getOscMessageAddress();
		
		ofxRTLS tracker;

	private:
		// OSC Sender
		ofxOscSender sender;
		string oscHost = "127.0.0.1";
		int oscPort = 8282;
		string messageAddress = "/rtls";

		//bool bForceSendID = false;
		//bool bForceSendPosition = false;
		//bool bForceSendOrientation = false;

		void threadedFunction();
		uint64_t lastSend = 0;
		int stopGap = 100; // number of milliseconds before we decide no data is being sent
		bool bSending = false;
		ofxOscMessage lastMessage;

		bool bOscEnabled = true;
};

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:
	OSCThread oscThread;
};