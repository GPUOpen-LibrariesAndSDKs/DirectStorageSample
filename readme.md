# DirectStorageSample 

This sample demonstrates the advantages of using DirectStorage instead of standard file I/O for loading assets. It shows the API and changes required to make such a pipeline work. 

The following are high-level examples of what can be explored with this sample:
- DirectStorage with compressed or uncompressed assets vs Win32 I/O with traditional run-time decompression
- Compression Options: Compression levels, and decompression location (CPU vs GPU)
- Bandwidth amplification affect
- Performance impact on rendering when streaming in resources
- Impact of cancelling requests when using DirectStorage to conserve bandwidth and processing cycles at the decompression location.

This sample provides profiling output [on-screen](#profiler-window-f2), through [CSV files](#built-in-profiling-output), and through PIX markers. 

All of the above can be controlled and measurements can be taken via [command-line options](#command-line-options). Some of the controls are also available at run-time.


# Screenshot

![Animatedscreenshot](images/animatedscreenshot.webp)


# How
The program uses a [list of assets and bounding volumes](src/Common/DirectStorageSample.json). When the camera enters a bounding volume, the asset associated with that bounding volume begins loading. The asset appears on screen as soon as loading is complete. When the camera exits the bounding volume, the asset disappears and it is unloaded.

If the camera exits a bounding volume for a given asset prior to loading, it will not become visible. The loading of an asset could continue until it is loaded even if it never becomes visible, or loading and associated requests can be cancelled so that loading never completes to conserves bandwidth and enable more relevant assets to load. Cancellation of a loading requests is supported via [command-line option](#allow-cancellation-allowcancellation).

# Prerequisites

To build DirectStorageSample, you must first install the following tools:

- [CMake 3.2x](https://cmake.org/download/)
- [Visual Studio 2019/2022](https://visualstudio.microsoft.com/downloads/)
- [Windows 10 SDK 10.0.18362.0](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)

# Simplified Build/Run Instructions
## DirectStorage with Compressed Assets

1. Run Build.bat
2. Run BuildMediaCompressed.bat
3. Run RunDirectStorage.bat

## DirectStorage with Non-Compressed Assets

1. Run Build.bat
2. Run BuildMediaUncompressed.bat
3. Run RunDirectStorage.bat

## No DirectStorage

1. Run Build.bat
2. Run RunNoDirectStorage.bat


# More Detailed Build Instructions

## Code

Run BuildCode.bat

This will create the sample solution and build the RelWithDebInfo configuration of the sample.

## Assets

Running with DirectStorage requires pre-processed assets. The assets may or may not be compressed.

- For compressed assets, run BuildMediaCompressed.bat.
- For non-compressed assets, run BuildMediaUnCompressed.bat.

These scripts are examples of how to use [TextureConverter.exe](#textureconverterexe).

# Running

- RunDirectStorage.bat  - Runs DirectStorage with whatever assets were last built (compressed or uncompressed).

- RunNoDirectStorage.bat - Runs without DirectStorage using standard compressed assets that are decoded on CPU.

These batch files are samples of how to run the sample. They can be used as-is or modified. They are currently used by profile.bat, which is an example of how to [automate profiling](#automated-profiling) the sample.

# Notes

- Only textures are processed by DirectStorage in this sample at this time.

- The profiling window shows the load time of each asset that has been streamed in. Multiple assets may have the same name if they are used twice in the scene.  

- At the moment, if you intend to test DirectStorage with and without compression, then you will need to re-run the appropriate BuildMedia*.bat file to produce either the compressed or uncompressed assets. Each time these batch files are run, they overwrite the prior version of the content.

- The TextureConverter.exe program compresses assets in formats and compression levels it supports. See the [command-line options for TextureConverter.exe](#textureconverterexe) for options. Run it to see available options. Examples for running it are located in BuildMediaCompressed.bat and BuildMediaUncompressed.bat.


# Command-line Options 

## DirectStorage_DX12.exe
Command-line options are formatted as JSON and are case-sensative.

Example:

`{"directstorage": true, "profile": true, "profileOutputPath":"DSOn.csv", "iotiming":true}`


### DirectStorage Options
---

#### __Direct Storage On/Off (directstorage)__
`{"directstorage:":<true|false>}`

Default: false

Whether or not to use DirectStorage to load assets. Compression for assets is determined by the TextureConverter utility and the options it is run with before the demo is run with this option set to true.

#### __Placed Resources (placedresources)__

`{"placedresources":<true|false>}`

Default: false

When true, placed resources will be used for DirectStorage loaded assets. When false, committed resources will be used instead.

#### __Staging Buffer Size (stagingbuffersize)__    

`{"stagingbuffersize":<size in bytes>}`

Default: 192MiB

Staging buffer size should be set commensurate to assets, compression technique, compression ratio, performance goals, and memory constraints. The default is currently 192MiB to accommodate systems with a small amount of VRAM while still being capable of loading all sample assets in this demo. For a system with significant VRAM >= 8GiB, a reasonable staging buffer size would be >= 512MiB.

Example: `{"stagingbuffersize":536870912}`

#### __Queue Length (queuelength)__

Default: 128

Set queue length to a reasonable size to allow auto-submit to prevent delaying DirectStorage requests from being submitted to the API. Auto-submit triggers at half queue length. A program could set queue length very large and call submit manually at its own defined rate, and should call submit once it knows outstanding requests are all submitted to the queue. If queue length is set too small, and the queue's capacity is reached, it will cause any subsequent direct storage requests to block until there's room in the queue for the new request.

#### __Allow Cancellation (allowcancellation)__

`{"allowcancellation":<true|false>}`

Default: false

When true, if the camera exits the streaming volume of an asset before the asset is loaded, then all DirectStorage requests associated with that asset's DirectStorage requests will attempt cancellation. When false, DirectStorage requests associated with an asset will continue, even after the camera has exited the streaming volume, and the asset hasn't fully loaded yet.

This option can conserve bandwidth, processing power, and increase the probability that other more recent load requests complete sooner. It can prevent the application from "falling behind" or processing data that is no longer relevant in the current view.

#### __Disable GPU Decompression (disablegpudecompression)__

`{"disablegpudecompression":<true|false>}`

Default: false

When true, and when using compressed assets with GPU-compatible compression algorithms, force DirectStorage to utilize the CPU instead of the GPU. This can be of benefit with very low-end GPUs if GPU decompression is slower than CPU decompression.


#### __Disable MetaCommand (disablemetacommand)__

`{"disablemetacommand":<true|false>}`

Default: false

When true, DirectStorage will use the reference implementation for GPU decompression rather than allow the device driver to select a hardware-specific optimized variant.

### Workload Options
---
#### __Mandelbrot Iterations(mandelbrotiterations)__

`{"mandelbrotiterations":<0,500000>}`

Default: 0

Generates a Mandelbrot fractal each frame to simulate an increased workload on the GPU. Higher values increase the workload. This is useful for measuring how GPU decompression interacts with greater compute or graphics workloads.

### Profiling Options
---
#### __Profile (profile)__

`{"profile":<true|false>}`

Default: false

Enabling profiling has very little impact on performance. This option saves out a CSV file that includes many run-time characteristics (load/streaming/run-time) of the application. The overhead is very low, and output is only saved when the application successfully closes.

#### __Profile Output Path (profileOutputPath)__

`{"profileOutputPath":"<file path>"}`

Default: "profilerOutput.csv"

Name of the CSV file when the profile option is set to true. Useful for scripting several runs in with different configurations and options.

Example: `{"profileOutputPath":"DSOn.csv"}`

#### __Profile Seconds(profileseconds)__

`{"profileseconds": <seconds>}`

Default: 0

How long to allow the program to execute before exiting. This is useful for scripting.

Example: `{"profileseconds": 500}`

#### __I/O Timing (iotiming)__

`{"iotiming":<true|false>}`

Default: false

When using DirectStorage, attempt to time the time of first enqueue to final processing of DirectStorage from all enqueued assets through the pipeline. May not be completely accurate. This option does not enable io timing for assets loaded when DirectStorage is not enabled or for any other assets using traditional I/O routines.

### Camera Options
---
#### __Camera Speed (cameraspeed)__

`{"cameraspeed":<[0.0,20.0]>}`

Default: 1.0

Speed of camera movement through the bounding volumes when autopilot is enabled.

#### __Camera Velocity Non-Linear (cameravelocitynonlinear)__

`{"cameravelocitynonlinear":<true|false>}`

Default: false

Whether to use ease-in/ease-out camera movement rather than constant velocity. For profiling, linear camera is more predictable for comparing results.


## TextureConverter.exe

---
```
Usage: TextureConverter.exe -configFile=<path to DirectStorageSample.json> -compressionFormat=<Compression Format> [-compressionLevel=<Valid Compression Level>] [-compressionExhaustive=<false|true>]
Compression Formats:
        none
        gdeflate

Compression Level:
        default (balance between compression ratio and compression performance)
        fastest (fastest to compress, but potentially lower compression ratio)
        best (highest compression ratio)

Compression Exhaustive:
        false (use the compressionLevel and compressionFormat specified -- default)
        true (Use the compression format and compression level with the best compression ratio. compressionLevel and compressionFormat specified are ignored)
```

Example 1 (Pre-process without compression): `bin\TextureConverter.exe -configFile=bin\DirectStorageSample.json -compressionFormat=none`

Example 2 (Pre-process with default compression): `bin\TextureConverter.exe -configFile=bin\DirectStorageSample.json -compressionFormat=gdeflate -compressionLevel=default`

Example 3 (Pre-process trying to find best compression level and format. *This is very slow*): `bin\TextureConverter.exe -configFile=bin\DirectStorageSample.json -compressionExhaustive`

# Controls Window (F1)

![Controls Window](images/controlswindowsmall.png)

### Autopilot Checkbox

- Checked - Camera movement is automatic.
- Unchecked - Camera movement is free with WASD+Mouse Look.

### Play Checkbox
- Checked - Animations, not including camera, enabled.
- Unchecked - Animations, not including camera, disabled.

### Speed Slider

Controls speed of animations and camera movement.

### Artificial Workload (Mandelbrot Iterations)
Adjusts the number of Mandelbrot iterations for generating a Mandelbrot fractal used as an artificial workload to simulate increased rendering or compute workloads as the sample's own rendering workload may not be substantial enough to see the impact of GPU decompression on other GPU workloads.

### Other Options
Other controls are useful to change postprocessing, debug modes, and presentation modes.

# Legend Window
The loading status of an asset is marked by the color of the bounding volume which the camera enters.

![Legend Window](images/legendonly.png)
## Run-time Asset States
Unloaded - The asset glTF file is not loaded. Memory for the asset has not been allocated.

Loaded - The asset is fully loaded. No additional loading operations are required.

Loading - The request to load the asset has been made due to the camera entering the bounding volume. The program has started making requests to load the glTF file, allocate memory, load shaders, vertex buffers, index buffers, and textures.

Unloading - The request to unload the asset has been made due to the camera exiting the bounding volume.

Cancelling  - When the [allow cancellation](#allow-cancellation-allowcancellation) command-line option is supplied, this signals that the loading request has been cancelled before it could be completed and is in the process of cancelling.

# Profiler Window (F2)

![Profiler Window](images/profilerwindowsmall.png)

Contains basic information about the system along with recent GPU timings and Scene Loading Timings.

## Scene Loading Timings

### Scene Name

The glTF&trade; scene or file which has been loaded and is visible. Multiple scenes may be loaded as the same time.

### CPU Load Time

The time, as measured by the CPU performance counter, from the moment the camera is inside the streaming volume to the moment the scene is loaded and about to become visible. 

When DirectStorage is enabled, some data is loaded through standard file I/O routines. This sample loads texture data with DirectStorage and all other data (glTF file and vertex data) using standard Win32 file I/O.

### IoTime (DirectStorage only)

IoTime measures the CPU time from the moment the first texture request is made to DirectStorage until the time transfer to the GPU is complete. This includes any decompression on the GPU or CPU by the DirectStorage API. 

When not using placed resources, this also includes the time it takes to perform memory allocation from CreateCommittedResource. The impact of memory allocations can be eliminated by enabling [placed resources](#placed-resources-placedresources).

DirectStorage request enqueue time is also included in this metric. If the queue capacity is too small, then it could take a considerable amount of time and distort the results for IoTime. To see the impact of queue capacity on the IoTime measurement, adjust [queue length](#)

### DirectStorage

On/Off - Indicates whether DirectStorage is in use or not. See command-line option documentation for details on how to enable/disable [DirectStorage](#direct-storage-onoff-directstorage).

### Placed Resources (DirectStorage only)

On/Off - Indicates whether placed resources were used for the glTF scene's textures. See command-line option documentation for details on how to enable/disable [placed resources](#placed-resources-placedresources).

### Texture File Size (DirectStorage only)

This size of the texture data stored on disk. If compression is enabled, then it will be the pre-processed compressed size; otherwise, it will be the pre-processed uncompressed size.

### Avg Disk Only Data Rate (DirectStorage only)

$Texture File Size / IoTime$. Requires i/o timing enabled. See the command-line option for [i/o timing](#io-timing-iotiming) for details on how to enable/disable seeing this value.

### Avg Amplified Data Rate (DirectStorage only)

$Uncompressed Data Size / IoTime$. If the assets were not compressed, then the Avg Amplified Data Rate is equal to the Avg Disk Only Data Rate. If the assets were compressed, the IoTime may be shorter, and the data rate is then going to be faster. Requires i/o timing enabled. See the command-line option for [i/o timing](#io-timing-iotiming) for details on how to enable/disable seeing this value.

# Built-in Profiling Output (CSV)

Built-in profiling output is useful for comparing results as the command-line options and hardware configurations are changed.

When using the [Profile option](#profile-profile), a CSV file is saved in the bin directory containing the same statistics as the [Profile Window](#profiler-window-f2). However, it also includes additional information:

## Mean Frame Time Before Loading (microseconds)

This is the average frame time of the last 60 frames before the asset started loading.

## Mean Frame Time During Loading (microseconds)

This is the average frame time of all frames from the time of the loading request until loading completes.

## Frame Count

The number of frames from loading request until loading completion.


# Automated Profiling

Profiling can be automated through batch files for comparing hardware and software configurations. [Profile.bat](profile.bat) is provided as an example, and it can be run as-is after running BuildCode.bat.

The defaults for this script do the following:
1. Run msinfo32.exe and dxdiag.exe and place their output into text files in the bin folder named with timestamps.
2. Pre-process assets *without* compression enabled.
3. Run the sample without DirectStorage enabled twice.
4. Run the sample with DirectStorage enabled twice.
5. Pre-process assets *with* compression enabled.
6. Run the sample with DirectStorage enabled twice. Now, with compressed assets.
7. Run the sample with DirectStorage enabled twice, but using the CPU for decompression rather than the GPU.

The script overrides the default staging buffer size, sets the run-time of each run to 500 seconds, sets the presentation mode to 0 (windowed), and the camera speed to half.

As configured, the script takes a bit over $8(runs) * 500(seconds)=66.67 minutes$ to complete. The results are stored in timestamped and named CSV files. These files can be opened or processed by your choice of program as they are [RFC 4180-compliant](!https://datatracker.ietf.org/doc/html/rfc4180).


# Performance Discussion

For a more thorough discussion about DirectStorage, its benefits and performance, see our GDC talk about it at https://www.youtube.com/watch?v=LvYUmVtOMRU.

## Overall Loading Performance

Loading performance is greatly influenced by several factors. For most current high-end systems, when only considering hardware, the bottlenecks occur at the storage device or the decompression device. However, software must keep these devices fed with data in order to realize these constraints.

Most modern storage devices have difficulty saturating the bus interface to the GPU as GPUs are usually attached to the bus with greater throughput than storage devices. For example, the storage device may be attached to the bus with a throughput of ~7.8GiB/s while the GPU is connected to the bus with a potential ~31GiB/s.

Given that the storage device is frequently the first location of limitation, keeping the data in compressed form enables the transfer of more data than would be possible if the data were uncompressed at the expense of the computation of decompression later on.

## Choice of Decompression Location
Decompression performance depends on the resources available on the chosen decompression device.

### CPU Decompression

Decompressing the data on the CPU uses CPU cycles and requires increased PCIe bus usage between the CPU and GPU. CPU decompression is the current norm for asset decompression. However, on most systems, the CPU is already busy with other workloads, processing the scene, generating rendering command-lists, game code, collision, etc. Streaming in assets seamlessly causes interruptions in processing other places, and could be difficult to schedule if there aren't enough resources available.

### GPU Decompression

The transfer from CPU to GPU will take less time as the data will be in compressed form. Decompressing data on the GPU requires more GPU cycles which could interrupt rendering frame rate. However, on most current mid-range and high-end systems, the difference in frame rate during decompression may have a lower impact than using the CPU for decompression. Furthermore, bandwidth from CPU to GPU is conserved. Decompressing data where it will be used increases overall efficiency. Also, because the data can be broken into several streams of data that may be decompressed in parallel, we can exploit the parallelism of the GPU to decompress the data.