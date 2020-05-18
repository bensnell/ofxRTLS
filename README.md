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

1. Include the relative path to `ofxRTLS` in your *addons.make* file, as usual. However, <u>do not</u> include the path to `ofxMotive` or `ofxOpenVRTracker` in your *addons.make* file. Then, regenerate your project files using OpenFrameworks' ProjectGenerator.

   *Note: If you plan on using any of the postprocessing options, include all of the Postprocessing-dependent addons defined above to the addons.make file.*

2. In Visual Studios, in the *Property Manager*, right click your entire project and select *Add Existing Property Sheet...*. Then, choose `ofxRTLS.props`. <u>Do not</u> include any other RTLS-related property sheets.

   *If your application does not directly interface with a tracking system, but instead receives data in the protobuf format, use only the `Protobuf.props` sheet instead.*

3. Instruct RTLS to build the tracking systems of your choice by passing the appropriate properties to MSBuild. Currently available tracking systems and their corresponding properties include those listed below. Tracking systems whose property equals `true` will be built. If a property equals `false`, it will not be built. Multiple systems can be built at once. If no systems are defined, ofxRTLS defaults to the Null System. Available systems include:

  | Tracking System          | Visual Studio Property | Supported <br />Platform | Notes                                                        |
  | ------------------------ | ---------------------- | ------------------------ | ------------------------------------------------------------ |
  | Optitrack Motive Tracker | RTLS_MOTIVE = true     | x64                      | 32-bit (x86) is not supported by Motive.<br />This uses Motive v2.2.0 |
  | OpenVR (e.g. HTC Vive)   | RTLS_OPENVR = true     | x64 (x86?)               |                                                              |
  | Null System              | RTLS_NULL = true       | x64, x86                 | This system exports fake data.                               |

  There are many ways to set a property in a Visual Studio project. It is important that this property must be defined before property sheets are imported in the project. The easiest way to add a property is to open up your **.vcxproj* file and add the following lines below (following XML structure) before property sheets are imported:

  ```xml
  <PropertyGroup>
      <!-- Instruct RTLS to build support for ofxMotive: -->
      <RTLS_MOTIVE>true</RTLS_MOTIVE>
  </PropertyGroup>
  ```

4. In your project *Properties* window, under *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions*, select *Edit* from the dropdown menu on the right and at the bottom of the window, check the box that says *Inherit from parent or project defaults*.

5. If you plan on using any of the postprocessing options (and have already included the appropriate addons according to the instructions in Step 1), pass the macro `RTLS_POSTPROCESS` in the Project Properties' *Preprocessor Definitions* <u>or</u> define `<RTLS_POSTPROCESS>true</RTLS_POSTPROCESS>` in **.vcproj* as above.

This approach to building ofxRTLS using defined properties allows TeamCity to build any configuration without regenerating project files, by passing different options to the compiler on the command line like so:

```bash
-p:RTLS_MOTIVE=true
```



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

The data exported over the RTLS-protocol protobuf format is detailed [here](https://github.com/local-projects/rtls-protocol). However, the context field will also contain useful information about the data's source. A trackable's `context` field will be a json string. When parsed, this json object possess the following information in key-value pairs:

| Key              | Possible Values (one of the following)                       |
| ---------------- | ------------------------------------------------------------ |
| `s` for "system" | `0` for Null System<br />`1` for OpenVR<br />`2` for Motive  |
| `t` for "type"   | `0` for markers (tracked objects)<br />`1` for reference (cameras, base stations, etc.) |

 

## Examples
Examples have been provided with and without postprocessing. Following Setup Step 3 above to change the example's tracking system.


## Troubleshooting

##### Cannot open include file in *ofxOpenVRTracker > libs > OpenVR > src > vrcommon > pathtools_public.h* and a few other files

When including ofxOsc in a project, sometimes the project generator overrides existing preprocessor definitions when it includes the definition `OSC_HOST_LITTLE_ENDIAN`. To prevent this, check the project's properties by right clicking on the project in the Solution Explorer and selecting *Properties*. Then, go to *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions* and add `%(PreprocessorDefinitions)`. It will likely end up looking like this afterwards:

```c++
OSC_HOST_LITTLE_ENDIAN
%(PreprocessorDefinitions)
```

