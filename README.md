[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/DroneEngage/droneengage_camera)

# DroneEngage Camera Module

This module uses native webrtc to stream videos from multiple camera to [WebClient](https://github.com/DroneEngage/droneengage_webclient "Weblient").


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/youtube_video_streaming.png?raw=true)](https://www.youtube.com/watch?v=hf5onZ2-7V4)

# Compile The Code

`de_camera` is built **as part of the WebRTC source tree** using `gn`/`ninja` — for both X86 and ARM (RPi) targets. The project is dropped into `webrtc/src/examples/de_camera`, and the `BUILD.gn` in this repo overwrites `webrtc/src/examples/BUILD.gn` to add `de_camera` as an extra example target. See [wiki/webrtc_build_setup.md](wiki/webrtc_build_setup.md) for the full, detailed walkthrough; this section is the quick reference.

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

If you build regularly, it's worth scripting this: a "sync" step that copies `src/` into the WebRTC tree, a "build" step that runs `ninja` against the target output directories, and a "collect" step that copies the resulting `de_camera` binaries back out into your own binaries folder. Deployment to a device (e.g. a Raspberry Pi) is then a plain `scp`/`rsync` of the binary plus its config files.

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

# Configuration

Camera and media behavior are configured in `de_camera.config.module.json` (see [`template.json`](template.json) for the full schema with defaults and descriptions). Selection and streaming settings are split into two top-level objects so the GCS config UI can edit either without overwriting the other:

- **`camera`** — which cameras to open. Two mutually exclusive styles:
  - **Scan range** — `camera_start_index` / `camera_end_index` scan `/dev/video<start..end>` and open every device that exists.
  - **Camera list** — an explicit `camera_list` array of `{ name, device_num | device_name }` entries, giving each camera a name that shows up in the WebClient.
- **`streaming`** — common resolution/framerate settings that apply to all cameras regardless of selection style.

### Resolution & framerate knobs (MAX, not exact)

The resolution/fps fields in `streaming` are **maximum/target** values, not exact requirements. The V4L2 driver picks the closest supported mode via `GetBestMatchedCapability`, so a camera with a lower native resolution simply uses its own max — you do not have to match the config to each camera.

| Field | Meaning |
| --- | --- |
| `capture_width` / `capture_height` / `capture_fps` | Resolution/fps requested from the V4L2 device (closest supported mode is picked). |
| `video_recording_fps` | Recording framerate passed to ffmpeg. `0` = use `capture_fps`. Acts as a max/target: if the camera delivers fewer fps, every frame is written as it arrives and ffmpeg's `-framerate` still matches the delivery rate closely enough for correct playback speed. Do not set this higher than the camera's actual fps. |
| `stream_max_width` / `stream_max_height` | WebRTC broadcaster downscale cap. Frames already smaller than this are broadcast as-is. `0` or `-1` = unlimited (no downscale). Protects RPi CPU/bandwidth. |
| `gcs_image_small_width` / `gcs_image_small_height` | Target size for the "small" GCS still image downlinked to GCS. If the camera's native resolution is already at or below this, the full-res image is sent. The locally saved still always keeps the full capture resolution. `0` or `-1` = unlimited (send full resolution to GCS as well). |

> **Recording fps sync (v5.0.0):** the recorder now derives its frame-sampling interval from `video_recording_fps` so it stays in sync with the `-framerate` passed to ffmpeg. Previously the interval came from a separate timer field; this could desync recorded playback speed from real time.

See [wiki/configuration.md](wiki/configuration.md) for the full field reference.

# Test / virtual camera scripts

The helper scripts under `scripts/` feed synthetic test patterns into a virtual V4L2 device so you can exercise `de_camera` without a real camera:

- `video_clock.sh [device_num]` — `testsrc2` clock pattern (default `/dev/video6`). Pass a device number, e.g. `./video_clock.sh 2` → `/dev/video2`.
- `video_colors.sh [device_num]` — `rgbtestsrc` color bars (default `/dev/video0`). Same device-number argument.
- `video_clock_others.sh` — the previous multi-device `ffmpeg` commands (1280x720, 320x200, 640x480), kept as a non-parameterized reference.

## To Install Scripts for Camera

See [scripts/README](https://github.com/DroneEngage/droneengage_camera/blob/master/scripts/README "Script Section") and [`scripts/compile_webrtc/`](scripts/compile_webrtc) for the raw compile scripts referenced above.
