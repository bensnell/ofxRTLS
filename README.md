# ofxRTLS

## Introduction

This addon is intended to act as a manager for any "Real Time Location System" and exports tracking information in a standardized format via the [`rtls-protocol`](https://github.com/local-projects/rtls-protocol). Currently, this addon supports Optitrack Motive's API and the OpenVR API.


### System Requirements

This requires a Windows 10, x64 computer since Optitrack maintains these requirements.

### Dependencies

Follow the instructions in each addon to include the relevant dependencies:

- [ofxMotive](https://github.com/local-projects/ofxMotive)
- [ofxOpenVRTracker](https://github.com/local-projects/ofxOpenVRTracker)
- [rtls-protocol (C++)](https://github.com/local-projects/rtls-protocol/tree/master/c%2B%2B)
    - Follow the Windows(x64) installation instructions to install the `protobuf` libraries system-wide.
    - The C++ headers are already copied into the `src` folder of this addon, but you may have to regenerate the headers if the version of `protobuf` that was installed is ahead of the version used to generate the headers (v3.11).

## How to use this addon with your project

First, make sure you have properly installed all of the dependencies; there are various idiosyncracies for each one. Ideally, make sure you can get the examples for each dependency up and running before starting to work with ofxRTLS.

### Setup

In order to declare which version of this addon you're using, declare one of the following in this Project's Preprocessor Definitions in the Property Sheets > C/C++ section:

- RTLS_OPENVR
- RTLS_MOTIVE

### Usage

Include the addon in your project as normal. Then create an ofxRTLS object, e.g.:

    ofxRTLS tracker;

Then, in your `setup()` method:

    tracker.setupParams(); // for ofxRemoteUI
    tracker.setup();
    tracker.start();
    ofAddListener(tracker.newFrameReceived, this, &ofApp::myHandlerFunction);

(name your handler whatever you think is most appropriate, e.g. `RTLSFrameReceived`)

Finally, implement your handler, e.g.:

    void ofApp::myHandlerFunction(RTLSEventArgs& args) {
        TrackableFrame frameReceived = args.frame;

        // Do whatever you need to do :)
    }

Take a look at the example for an in-depth look at using ofxRTLS with OpenVR. See below for instructions on switching the example to use Motive instead.

## Example - UPDATE TODO, examples are now split into two
The example is setup to use `RTLS_OPENVR`, but this can be changed:

- Change the Preprocessor Definitions (see "Setup" above) to have `RTLS_MOTIVE`
- Similarly, change the defined constant in `example/src/ofApp.h`
- Edit `addons.make` to remove the ofxOpenVRTracker addon and include ofxMotive
- "Clean" your project (potentially also delete the `.vs/` folder)
- Regenerate project files

You may have to troubleshoot further (see the [ofxMotive](https://github.com/local-projects/ofxMotive) README for help) and potentially regenerate project files. It may help, for example, to edit `addons.make` to remove the ofxOpenVRTracker addon since that isn't needed for ofxMotive.

## Troubleshooting

##### Cannot open include file in *ofxOpenVRTracker > libs > OpenVR > src > vrcommon > pathtools_public.h* and a few other files

When including ofxOsc in a project, sometimes the project generator overrides existing preprocessor definitions when it includes the definition `OSC_HOST_LITTLE_ENDIAN`. To prevent this, check the project's properties by right clicking on the project in the Solution Explorer and selecting *Properties*. Then, go to *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions* and add `%(PreprocessorDefinitions)`. It will likely end up looking like this afterwards:

```c++
OSC_HOST_LITTLE_ENDIAN
%(PreprocessorDefinitions)

```



## Reference

xxx

