# ofxRTLS

## Introduction

This addon is intended to act as a manager for any "Real Time Location System" and exports tracking information in a standardized format via the [`rtls-protocol`](https://github.com/local-projects/rtls-protocol). Currently, this addon supports Optitrack Motive's API and the OpenVR API.


### System Requirements

This requires a Windows 10, x64 computer since Optitrack maintains these requirements.

This has been developed with OpenFrameworks version 0.11.0

### Dependencies

Follow the instructions in each addon to include the relevant dependencies:

- [ofxMotive](https://github.com/local-projects/ofxMotive)
- [ofxOpenVRTracker](https://github.com/local-projects/ofxOpenVRTracker)
- [rtls-protocol (C++)](https://github.com/local-projects/rtls-protocol/tree/master/c%2B%2B)
    - Follow the Windows(x64) installation instructions to install the `protobuf` libraries. **Do not install them system-wide.**
    - The C++ headers are already copied into the `src` folder of this addon, but you may have to regenerate the headers if the version of `protobuf` that was installed is ahead of the version used to generate the headers (v3.11.3).

The addon also supports optional post-processing of the data. Options include:

- ML-enabled bit sequence identification
- ID re-mappings
- Smoothing and filtering (easing, kalman)
- Predictive gap-filling filters (continuity)

These post-processing options require additional addons, including:

- ofxOpenCv (comes with OF v0.11.0)
- ofxCv (branch: [project/lp.rtls-server](project/lp.rtls-server))
- ofxFDeep (branch: [fdeep-v0.12.1-p0](https://github.com/local-projects/ofxFDeep/tree/fdeep-v0.12.1-p0))
- [ofxFilter](https://github.com/local-projects/ofxFilter/tree/master)

## How to use this addon with your project

First, make sure you have properly installed all of the dependencies; there are various idiosyncrasies for each one. Ideally, make sure you can get the examples for each dependency up and running before starting to work with ofxRTLS.

### Setup

include the relative path to `ofxRTLS` in your *addons.make* file, as usual. Regenerate your project files using OpenFrameworks' ProjectGenerator.

In Visual Studios, in the *Property Manager*, add the the appropriate property sheet to your project. Each mode of server operation has a different property sheet. Only include one of these. Available property sheets are in the *ofxRTLS* directory and include:

- *RTLS_OPENVR.props* for an OpenVR server. This will automatically include the macro `RTLS_OPENVR`. ***Note: OpenVR does not currently work with the filtering dependencies.***
- *RTLS_MOTIVE.props* for an Optitrack Motive server. This will automatically include the macro `RTLS_MOTIVE`. 

In your project *Properties* window, under *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions*, select *Edit* from the dropdown menu on the right and at the bottom of the window, check the box that says *Inherit from parent or project defaults*.

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

Take a look at the examples for a more in-depth look at using ofxRTLS with ofxMotive and ofxOpenVRTracker.

## Examples
There are two examples, one for Motive and one for OpenVR. In fact, the two examples are nearly identical; the only two differences being:

- the defined constant in `example/src/ofApp.h`
- which one of either ofxOpenVRTracker or ofxMotive is included in `addons.make`

To run the examples, first declare the appropriate Preprocessor Definition (see "Setup"). Then follow the specific setup instructions for whichever addon you are using (ofxMotive or ofxOpenVRTracker). For example, you will need to copy the required Motive libraries, and profile and calibration files into the `bin` directory if you are using ofxMotive.


## Troubleshooting

##### Cannot open include file in *ofxOpenVRTracker > libs > OpenVR > src > vrcommon > pathtools_public.h* and a few other files

When including ofxOsc in a project, sometimes the project generator overrides existing preprocessor definitions when it includes the definition `OSC_HOST_LITTLE_ENDIAN`. To prevent this, check the project's properties by right clicking on the project in the Solution Explorer and selecting *Properties*. Then, go to *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions* and add `%(PreprocessorDefinitions)`. It will likely end up looking like this afterwards:

```c++
OSC_HOST_LITTLE_ENDIAN
%(PreprocessorDefinitions)
```
