# DroneEngage Camera Module

This module uses native webrtc to stream videos from multiple camera to [WebClient](https://github.com/DroneEngage/droneengage_webclient "Weblient").


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/youtube_video_streaming.png?raw=true)](https://www.youtube.com/watch?v=hf5onZ2-7V4)

# Compile The Code


## for X86

first you need webrtc compiled code to get libwebrtc.a 
you need to use branch 

`git ls-remote https://chromium.googlesource.com/external/webrtc --heads refs/remotes/branch-heads/7103`

this is an old branch and you need **Ubuntu 18.04** to build it.

Once you have **libwebrtc.a ** just put it in  **/lib/webrtc94-local/lib/x64/Debug** and run 

you can download a compiled version from [here](https://drive.google.com/file/d/10KvinexvvDRUgd6afGLAbMCgCr_h8yM6/view?usp=sharing "here")
`make debug`

## for Arm

We compile de_camera as part of the WEBRTC project itself. We compiled it as an example next to the examples that already exist.

use BUILD.gn file in this project to overwrite the example BUILD.gn in webrtc source, and add the current project file inside **src** in a folder called de_camera similar to:

/home/webrtc/webrtc/webrtc-checkout/src/examples/de_camera


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/virtual_machine_de_camera.png?raw=true)](https://github.com/DroneEngage/droneengage_camera/blob/master/res/virtual_machine_de_camera.png?raw=true)

then compile webrtc and it will compile project de_camera as part of the project.
then you can use it in RPI or JetsonNano

## To Install Scripts for Camera

Please check [Script Section](https://github.com/DroneEngage/droneengage_camera/blob/master/scripts/README.md "Script Section")




    sudo apt-get install nlohmann-json3-dev
    sudo apt-get install libjsoncpp-dev # Install the development package
    sudo apt-get install libjpeg-dev  # Install the development package
    sudo apt-get install libx11-dev libexpat1-dev 

    sudo apt-get install clang 
    sudo apt-get install libc++-dev


## How to prepare WEBRTC environment

On Ubuntu 22.04

    conda create -n webrtc_7103 python=3.10
    conda env activate webrtc_7103
    
    
    sudo apt update
    sudo apt install build-essential
    sudo apt install python-is-python3
    sudo apt install curl
    sudo apt install libxss-dev
    sudo apt install libasound2-dev
    sudo apt install libpulse-dev
    sudo apt install libvpx-dev
    sudo apt install libopus-dev
    sudo apt install libv4l-dev
    sudo apt install libfontconfig1-dev
    sudo apt install libxtst-dev
    sudo apt install libgtk-3-dev


    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    export PATH="$PATH:/path/to/depot_tools"
    mkdir webrtc
    cd webrtc
    fetch --nohooks webrtc
    cd src
    git ls-remote https://chromium.googlesource.com/external/webrtc --heads branch-heads/7103
    git checkout -b refs/branch-heads/7103
    gclient sync

    build/linux/sysroot_scripts/install-sysroot.py --arch=amd64

    gn gen out/Default


### To Compile for X86

    python build/linux/sysroot_scripts/install-sysroot.py --arch=amd64

    gn gen out/m137_x86 --args='is_debug=true use_system_libjpeg=false treat_warnings_as_errors=false  rtc_include_tests=false rtc_use_h264=true  rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="x64" is_clang=true  '



### To compile for RPI-5 / RPI-4 / RPI-Z2 W

    python build/linux/sysroot_scripts/install-sysroot.py --arch=arm64

    gn gen out/rpi-5_d --args='is_debug=true use_system_libjpeg=true treat_warnings_as_errors=false  rtc_include_tests=false rtc_use_h264=true  rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="arm64" is_clang=true  '



