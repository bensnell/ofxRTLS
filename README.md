# ofxRTLS

## Introduction

This addon is intended to act as a manager for any "Real Time Location System" and exports tracking information in a standardized format via the [`rtls-protocol`](https://github.com/local-projects/rtls-protocol). Currently, this addon supports Optitrack Motive's API and the OpenVR API.

This addon also supports optional realtime post-processing of the data. Options include:

- ML-enabled bit sequence identification
- ID re-mappings
- Smoothing and filtering (easing, kalman)
- Predictive gap-filling filters (continuity)
- etc.


### System Requirements

This requires a Windows 10, x64 computer since Optitrack maintains these requirements.

This has been developed with OpenFrameworks version 0.11.0

### Dependencies

#### General Dependencies

[rtls-protocol (C++)](https://github.com/local-projects/rtls-protocol/tree/master/c%2B%2B)

- The rtls-protocol uses Protobuf to package data in a efficient format that can be easily transmitted across networks and unpacked in virtually any language. Protobuf v3.11.3 has already been included in this repo for Windows development. The runtime libraries are in the folder *protobuf* and the C++ headers have been copied to the *src* folder. Other versions of Protobuf may require header regeneration and inclusion of different libraries.

#### Tracking System-Specific Dependencies

- Motive (Optitrack)
    - Using Motive requires the addon [ofxMotive](https://github.com/local-projects/ofxMotive/tree/version/2.2.0) (branch: `version/2.2.0`). See its repo for any additional dependencies that are not listed here.
    - [Motive](https://www.optitrack.com/downloads/motive.html) must be installed on the system you are working on and a valid license key generated and stored on your computer. If not, ofxRTLS will not compile and/or Motive will not connect to the Motive API.
- OpenVR (e.g. HTC Vive, etc.)
    - Using OpenVR requires the addon [ofxOpenVRTracker](https://github.com/local-projects/ofxOpenVRTracker). See its repo for any additional dependencies that are not listed here.

#### Post-Processing Dependencies

If the macro `RTLS_ENABLE_POSTPROCESS` is defined, then ofxRTLS expects a number of additional dependencies. Make sure they are included in the *addons.make* file of your project. See the *example_motive_postprocess/addons.make* for an example using Motive.

- ofxOpenCv (this comes with OF v0.11.0)
- [ofxCv](https://github.com/local-projects/ofxCv/tree/project/lp.rtls-server) (branch: `project/lp.rtls-server`)
- [ofxFDeep](https://github.com/local-projects/ofxFDeep/tree/fdeep-v0.12.1-p0) (branch: `fdeep-v0.12.1-p0`)
- [ofxFilter](https://github.com/local-projects/ofxFilter/tree/master)

## How to use this addon with your project

First, make sure you have properly installed all of the dependencies; there are various idiosyncrasies for each one. Ideally, make sure you can get the examples for each dependency up and running before starting to work with ofxRTLS.

### Setup

1. Include the relative path to `ofxRTLS` in your *addons.make* file, as usual. Regenerate your project files using OpenFrameworks' ProjectGenerator.

2. In Visual Studios, in the *Property Manager*, right click your entire project and select *Add Existing Property Sheet...*. Then, choose the appropriate sheet. Available property sheets are listed below and included in the *ofxRTLS* directory. 

   *Note: If your application directly interfaces with a tracking system, choose the corresponding sheet `RTLS_[SYSTEM].props`. If your application does not directly interface with a tracking system, but instead receives data in the protobuf format, use the `Protobuf.props` sheet instead.*

   *Note: Only one of these property sheets may be added to a project. Each application that uses ofxRTLS may only directly interface with a single tracking system. For example, to interface with both Motive and OpenVR, two applications would be needed.*

| Property Sheet      | Tracking System        | Supported <br />Configuration | Supported <br />Platform | Notes                                                        |
| ------------------- | ---------------------- | ----------------------------- | ------------------------ | ------------------------------------------------------------ |
| `RTLS_MOTIVE.props` | Motive (Optitrack)     | Release, Debug                | x64                      | 32-bit (x86) is not supported by Motive.<br />This uses Motive v2.2.0 |
| `RTLS_OPENVR.props` | OpenVR (e.g. HTC Vive) | Release, Debug                | x64 (x86?)               |                                                              |
| `Protobuf.props`    | None                   | Release, Debug                | x64, x86                 |                                                              |

3. In your project *Properties* window, under *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions*, select *Edit* from the dropdown menu on the right and at the bottom of the window, check the box that says *Inherit from parent or project defaults*.

4. If you plan on using any of the postprocessing options, pass the macro `RTLS_ENABLE_POSTPROCESS` in the Project Properties' *Preprocessor Definitions*.

### Usage

Include the addon in your project as normal. Then create an ofxRTLS object, e.g.:

    ofxRTLS tracker;

Then, in your `setup()` method:

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
There are examples for Motive and for OpenVR. When possible, another example has been provided with postprocessing enabled. The only differences between the examples include:

- The Property Sheet included, either `RTLS_MOTIVE.props` or `RTLS_OPENVR.props`
- The Addons included in `addons.make`
- The definition of `RTLS_ENABLE_POSTPROCESS`

To run the examples, follow the specific setup instructions for whichever addon you are using, then run.


## Troubleshooting

##### Cannot open include file in *ofxOpenVRTracker > libs > OpenVR > src > vrcommon > pathtools_public.h* and a few other files

When including ofxOsc in a project, sometimes the project generator overrides existing preprocessor definitions when it includes the definition `OSC_HOST_LITTLE_ENDIAN`. To prevent this, check the project's properties by right clicking on the project in the Solution Explorer and selecting *Properties*. Then, go to *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions* and add `%(PreprocessorDefinitions)`. It will likely end up looking like this afterwards:

```c++
OSC_HOST_LITTLE_ENDIAN
%(PreprocessorDefinitions)
```

