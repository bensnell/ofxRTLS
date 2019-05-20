# ofxRTLS

## Introduction

This addon is intended to act as a manager for any RealTime Location System and as an OSC sender of 3D tracking data. Currently, this addon only integrates Optitrack Motive's API.

An attempt was made to integrate Vive Tracking, but proved unsuccessful for a few reasons:

1. Optitrack's dlls and Vive's dlls must be in the same directory as the executable; however, they require different versions of the same dlls, which produces conflicts (Entry Point errors).
2. Vive loads libraries twice -- this seems less a problem with inegration with RTLS and more a problem with the addon ofxOpenVRTracker itself.

### Systems

This requires Windows 10, x64 computer since Optitrack maintains these requirements.

### Dependencies

Follow the instructions in each addon to include the relevant dependencies:

- ofxMotive
- ofxOpenVRTracker

#### Installation

Follow the installation instructions for each addon.

## How to use this addon with your project

See the examples provided for instructions on how to use this with each tracking system.

#### Setup

In order to declare which version of this addon you're using, declare one of the following in this Project's Preprocessor Definitions in the Property Sheets > C/C++ section:

- RTLS_VIVE
- RTLS_MOTIVE

## Troubleshooting

##### Cannot open include file in *ofxOpenVRTracker > libs > OpenVR > src > vrcommon > pathtools_public.h* and a few other files

When including ofxOsc in a project, sometimes the project generator overrides existing preprocessor definitions when it includes the definition `OSC_HOST_LITTLE_ENDIAN`. To prevent this, check the project's properties by right clicking on the project in the Solution Explorer and selecting *Properties*. Then, go to *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions* and add `%(PreprocessorDefinitions)`. It will likely end up looking like this afterwards:

```c++
OSC_HOST_LITTLE_ENDIAN
%(PreprocessorDefinitions)

```



## Reference

xxx

