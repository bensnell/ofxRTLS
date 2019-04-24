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

#### Installation

Follow the installation instructions for each addon.

## How to use this addon with your project

See the example provided, specifically ofApp.cpp, for instructions on how to use this.

## Troubleshooting

xxx

## Reference

xxx

