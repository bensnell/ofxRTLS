# ofxRTLS

## Introduction

This addon is intended to act as a manager for any "Real Time Location System" and exports tracking information in a standardized format via the [`rtls-protocol`](https://github.com/local-projects/rtls-protocol). Currently, this addon supports Optitrack Motive's API and the OpenVR API.

This addon also supports optional realtime post-processing of the data. Options include:

- ML-enabled bit sequence identification
- ID re-mappings
- Smoothing and filtering (easing, kalman)
- Predictive gap-filling filters (continuity)
- Hungarian Algorithm / Tracking
- etc.

The addon also allows data to be saved and played back from the [C3D](https://www.c3d.org/) file format.


### System Requirements

This requires a Windows 10, x64 computer if using the Optitrack Motive system.

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

If the macro `RTLS_POSTPROCESS` is defined, then ofxRTLS expects a number of additional dependencies. Make sure they are included in the *addons.make* file of your project. See the *example_motive_postprocess/addons.make* for an example using Motive.

- ofxOpenCv (this comes with OF v0.11.0)
- [ofxCv](https://github.com/local-projects/ofxCv/tree/project/lp.rtls-server) (branch: `project/lp.rtls-server`)
- [ofxFDeep](https://github.com/local-projects/ofxFDeep/tree/fdeep-v0.12.1-p0) (branch: `fdeep-v0.12.1-p0`)
- [ofxFilter](https://github.com/local-projects/ofxFilter/tree/master)
- [ofxHungarian](https://github.com/local-projects/ofxHungarian)

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
6. If you plan on using the recording or playback functionality, pass the macro `RTLS_PLAYER` in the Project Properties' *Preprocessor Definitions* <u>or</u> define `<RTLS_PLAYER>true</RTLS_PLAYER>` in **.vcproj* as above.

This approach to building ofxRTLS using defined properties allows MSBuild (and CI pipelines like TeamCity) to build any configuration without regenerating project files, by passing different options to the compiler on the command line like so:

```bash
-p:RTLS_MOTIVE=true
```

You can pass multiple options to the compiler like below. This would build the NULL and MOTIVE systems with postprocessing and recorder/player enabled.

```bash
-p:RTLS_NULL=true;RTLS_MOTIVE=true;RTLS_OPENVR=false;RTLS_POSTPROCESS=true;RTLS_PLAYER=true
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

#### Data

The data exported over the RTLS-protocol protobuf format is detailed [here](https://github.com/local-projects/rtls-protocol). 

An `id` of `0` is not allowed, since it is the default field value for the RTLS Protocol protobuf objects. Using 0 may result in unexpected behaviors.

The context field may also contain additional useful information. This field will always be a utf-8 parseable json string. 

The `TrackableFrame`'s context field will always contain information about the data's source. When parsed, this json object possess the following information in key-value pairs:

| Key  | Key Meaning   | Values (one of the following)                                |
| ---- | ------------- | ------------------------------------------------------------ |
| `s`  | <u>s</u>ystem | `0` for Null System<br />`1` for OpenVR<br />`2` for Motive  |
| `t`  | <u>t</u>ype   | `0` for markers (tracked objects)<br />`1` for reference, contributors, or observers (cameras, base stations, etc.) |

The `Trackable`'s context field may or may not contain any information. If it does, these are the possible keys and associated values.

| Key  | Key Meaning                     | Values (one of the following)                |
| ---- | ------------------------------- | -------------------------------------------- |
| `m`  | <u>m</u>ight need recalibration | `0` for false (by default)<br />`1` for true |

## Examples

Examples have been provided with and without postprocessing. Following Setup Step 3 above to change the example's tracking system.



## Postprocessing Options

If postprocessing is enabled, the following actions are available and can be individually toggled ON and OFF. They will be executed in this order. See below for more documentation on each action.

| Action                                 | Description                                                  |
| -------------------------------------- | ------------------------------------------------------------ |
| Map IDs                                | Map the `ID` parameter from one value to another, using a json dictionary supplied in the data folder. |
| Remove Unidentifiable Before Hungarian | Remove trackables that are unable to be identified. A trackable with an `id <= 0`, no `cuid`, and no `name`, will be considered unidentifiable. |
| Apply Hungarian                        | Use the Hungarian (linear assignment) algorithm to track objects and provide continuity to identifiable information. See below for a detailed explanation of options. |
| Remove Unidentifiable Before Filters   | Same as above, but applied again, before Filters.            |
| Apply Filters                          | Apply smoothing and filtering to all remaining trackables with adjustable sets of filter operators. |

### Map IDs

Parameters include:

| Parameter      | Description                  |
| -------------- | ---------------------------- |
| `ID Dict Path` | Path to the json dictionary. |

The json dictionary should resemble this structure, where  `2 ** nBits` indicates the number of values in the dictionary. The `dict` contains a list of this length, where each index will be a key mapped to the value in its spot in the list. For example, the ID `0` maps to the value `-1`, while the ID `5` maps to the value `10`.

```json
{
   "nBits":3,
   "dict":[
      -1,
      2,
      4,
      6,
      8,
      10,
      12,
      14
   ]
}
```

### Remove Unidentifiable (at multiple locations)

Remove any trackables which cannot be identified. Unidentifiable trackables have the `TrackableKeyType` `none`. The key type is found using the following function.

```C++
TrackableKeyType getTrackableKeyType(const Trackable& t) {
	if (t.id() > 0) return KEY_ID;
	if (!t.cuid().empty()) return KEY_CUID;
	if (!t.name().empty()) return KEY_NAME;
	return KEY_NONE;
}
```
Note that zero and negative `id`'s are considered invalid and will be removed by this action.

### Apply Hungarian

Parameters include:

| Parameter                 | Description                                                  |
| ------------------------- | ------------------------------------------------------------ |
| `Temporary Key Types`     | A string list of temporary key types, comma-delimited. Key types include: `id`, `cuid`, `name`, `none`. A trackable with a temporary key contains a key that is likely to change. The trackable is not defined by its key; rather, it is passively identified by its key. |
| `Permanent  Key Types`    | A string list of permanent key types, comma-delimited. Key types include: `id`, `cuid`, `name`, `none`. A trackable with a permanent key contains a key that will not change. The trackable is defined by its key. |
| `Item Radius`             | In the trackable's native units, the approximate radius of a trackable. This is used for calculating the intersection over union within the Hungarian solver. A smaller radius will result in greater sensitivity of cost, but may lose out on spatial relations and dependencies. |
| `From Dataset Permanance` | The permanence of samples used in the Hungarian solver's `FROM` dataset. An enumerated value with the available values `TEMPORARY`, `PERMANENT`, `TEMPORARY_AND_PERMANENT`.  By default, this should be `TEMPORARY`. Including `PERMANENT` will provide additional, perhaps superfluous content, for the solver. If included, it is recommended not to allow remapping using permanent keys (see below parameters). |
| `To Dataset Permanance`   | The permanence of samples used in the Hungarian solver's `TO` dataset. An enumerated value with the available values `TEMPORARY`, `PERMANENT`, `TEMPORARY_AND_PERMANENT`.  By default, this should be `TEMPORARY`. Including `PERMANENT` will provide additional, perhaps superfluous content, for the solver. If included, it is recommended not to allow remapping using permanent keys (see below parameters). |
| `Map Recursion Limit`     | The recursion limit of the key mapping search.               |
| `Remove Matching Keys`    | Remove trackables whose keys match from the dataset before passing to the solver. Including these trackables may provide additional, superfluous context to the solver, but may also open you up to vulnerabilities in trackable matching. By default, this should be `true`. |
| `Assign CUIDs to UnID`    | Assign a ` cuid` to an unidentifiable trackable. By default, this value is `false`. Only set to `true` if you plan to provide the postprocessor with trackables without IDs and would like to track them. |
| `CUID Start Counter`      | What positive integer value should assigned `cuid`'s start at? |
| `Allow Remap From Perm`   | Are remappings from permanent key types allowed? If so, it is highly recommended that you remove matching keys before solve, since this will prevent permanent IDs from separating during tracking. By default, this value is `false`. Setting to `true` under most circumstances defeats the purpose of distinguishing temporary and permanent key types. |
| `Allow Remap To Perm`     | Are remappings to permanent key types allowed? By default, this value is `false`. Setting to `true` under most circumstances defeats the purpose of distinguishing temporary and permanent key types. |



### Apply Filters

Filter parameters are documented [here](https://github.com/local-projects/ofxFilter).




## Troubleshooting

##### Cannot open include file in *ofxOpenVRTracker > libs > OpenVR > src > vrcommon > pathtools_public.h* and a few other files

When including ofxOsc in a project, sometimes the project generator overrides existing preprocessor definitions when it includes the definition `OSC_HOST_LITTLE_ENDIAN`. To prevent this, check the project's properties by right clicking on the project in the Solution Explorer and selecting *Properties*. Then, go to *Configuration Properties  > C/C++ > Preprocessor > Preprocessor Definitions* and add `%(PreprocessorDefinitions)`. It will likely end up looking like this afterwards:

```c++
OSC_HOST_LITTLE_ENDIAN
%(PreprocessorDefinitions)
```

