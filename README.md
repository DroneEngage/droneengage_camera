[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/DroneEngage/droneengage_camera)

# DroneEngage Camera Module

This module uses native webrtc to stream videos from multiple camera to [WebClient](https://github.com/DroneEngage/droneengage_webclient "Weblient").


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/youtube_video_streaming.png?raw=true)](https://www.youtube.com/watch?v=hf5onZ2-7V4)

# Compile The Code

`de_camera` is built **as part of the WebRTC source tree** using `gn`/`ninja` — for both X86 and ARM (RPi) targets. The project is dropped into `webrtc/src/examples/de_camera`, and the `BUILD.gn` in this repo overwrites `webrtc/src/examples/BUILD.gn` to add `de_camera` as an extra example target. See [wiki/webrtc_build_setup.md](wiki/webrtc_build_setup.md) for the full, detailed walkthrough (including what each script in `local/` does); this section is the quick reference.

## Current WebRTC version

We currently build against **WebRTC/Chromium M137**, branch `branch-heads/7151`:

    git ls-remote https://chromium.googlesource.com/external/webrtc --heads refs/remotes/branch-heads/7151

(older builds used `branch-heads/7103` on Ubuntu 18.04 — that path is deprecated, see "Legacy build" below.)

## 1. Install de_camera build dependencies

    sudo apt-get install nlohmann-json3-dev
    sudo apt-get install libjsoncpp-dev  # Install the development package
    sudo apt-get install libjpeg-dev     # Install the development package
    sudo apt-get install libx11-dev libexpat1-dev

    sudo apt-get install clang
    sudo apt-get install libc++-dev

## 2. Prepare the WebRTC environment (once)

On Ubuntu 22.04:

    conda create -n webrtc_7151 python=3.10
    conda activate webrtc_7151

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
    git checkout -b local_7151 refs/remotes/branch-heads/7151
    gclient sync

    # Install sysroots for the architectures you plan to build
    build/linux/sysroot_scripts/install-sysroot.py --arch=amd64
    build/linux/sysroot_scripts/install-sysroot.py --arch=arm64

    gn gen out/Default

## 3. Add de_camera to the WebRTC tree

Copy this project's `src/` into `webrtc/src/examples/de_camera`, and overwrite `webrtc/src/examples/BUILD.gn` with the `BUILD.gn` from this repo.

If you build regularly, use the automation scripts in [`local/`](local/) instead of doing this by hand (edit the paths at the top of each script to match your own WebRTC checkout first):

    local/sh_update_code.sh    # copies src/ -> webrtc/src/examples/de_camera
    local/sh_buildme.sh        # builds amd64 + arm64 (pass `amd` or `arm` to build just one)
    local/sh_copy_binaries.sh  # copies built binaries into compiled_bin/ and compiled_rpi_bin/
    local/sh_do_all.sh         # runs all three of the above in sequence
    local/sh_ssh_copy_code.sh  # uploads the RPi binary + config to a device over scp

## 4. Generate build directories and build

Run from `webrtc/src`. Generate each target/configuration once, then build it with `ninja`.

### X86

    build/linux/sysroot_scripts/install-sysroot.py --arch=amd64

    # debug
    gn gen out/m137_x86_d --args='is_debug=true use_system_libjpeg=false treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="x64" is_clang=true'

    # release
    gn gen out/m137_x86_r --args='is_debug=false use_system_libjpeg=false treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="x64" is_clang=true'

    ninja -C out/m137_x86_r

### ARM64 (RPi-5 / RPi-4 / RPi-Z2 W)

    build/linux/sysroot_scripts/install-sysroot.py --arch=arm64

    # debug
    gn gen out/rpi-5 --args='is_debug=true use_system_libjpeg=true treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="arm64" is_clang=true'

    # release
    gn gen out/rpi-5_r --args='is_debug=false use_system_libjpeg=true treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="arm64" is_clang=true'

    ninja -C out/rpi-5_r

The compiled binary is `out/<target>/de_camera`. The same code/config also works on RPi-4 and RPi-Zero 2 W.

## Legacy build (deprecated)

Older versions of this project could also be compiled standalone against a prebuilt `libwebrtc.a` (see `lib/webrtc-local/` and the root `Makefile`, using `branch-heads/7103` on Ubuntu 18.04). This path is no longer maintained — use the gn/ninja workflow above instead.

## To Install Scripts for Camera

See [scripts/README](https://github.com/DroneEngage/droneengage_camera/blob/master/scripts/README "Script Section") and [`scripts/compile_webrtc/`](scripts/compile_webrtc) for the raw compile scripts referenced above.
