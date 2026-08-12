# How to Compile WebRTC & de_camera

This page is the detailed build/setup guide for `de_camera`. It expands on the quick-reference "Compile The Code" section in the top-level [README.md](../README.md).

---

## 1. How the build is structured

`de_camera` is **not** built standalone. It is built as one of the WebRTC `examples/` targets, the same way `peerconnection_client` or `AppRTCMobile` are:

- A full copy of this repo's `src/` is placed at `webrtc/src/examples/de_camera` inside a full WebRTC checkout.
- This repo's root `BUILD.gn` overwrites `webrtc/src/examples/BUILD.gn`. It is the stock WebRTC examples `BUILD.gn` with a `de_camera` target added alongside the other examples.
- `gn`/`ninja` then compile `de_camera` together with the rest of WebRTC, and link it against WebRTC's internal libraries directly (no separate `libwebrtc.a` packaging step needed).
- `WEBRTC_EXAMPLES` in the repo root is a convenience symlink to `webrtc/src/examples` on a dev machine — it lets you jump to the WebRTC examples folder without hard-coding the checkout path.

This is why the toolchain (Ubuntu version, GCC/clang, sysroots) has to match what WebRTC itself expects for the branch you're building.

## 2. WebRTC version currently used

We build against **WebRTC/Chromium milestone M137**, branch `branch-heads/7151` of `https://webrtc.googlesource.com/src.git`. This shows up in `gn gen` output directory names (`out/m137_x86_r`, etc.).

    git ls-remote https://chromium.googlesource.com/external/webrtc --heads refs/remotes/branch-heads/7151

Older documentation referenced `branch-heads/7103` built on Ubuntu 18.04 with a standalone `libwebrtc.a` — that toolchain is deprecated (see [Legacy build](#legacy-standalone-build-deprecated) below). If you need to bump the WebRTC version in the future, checking out a newer `branch-heads/<N>` tag and re-running `gclient sync` + the `gn gen` steps below is normally enough; the `de_camera` sources and `BUILD.gn` do not need to change unless WebRTC's public API changed.

## 3. Install de_camera's own dependencies

    sudo apt-get install nlohmann-json3-dev
    sudo apt-get install libjsoncpp-dev
    sudo apt-get install libjpeg-dev
    sudo apt-get install libx11-dev libexpat1-dev
    sudo apt-get install clang
    sudo apt-get install libc++-dev

## 4. Fetch and prepare WebRTC (one-time per machine)

Tested on **Ubuntu 22.04**.

    conda create -n webrtc_7151 python=3.10
    conda activate webrtc_7151

    sudo apt update
    sudo apt install build-essential python-is-python3 curl \
        libxss-dev libasound2-dev libpulse-dev libvpx-dev libopus-dev \
        libv4l-dev libfontconfig1-dev libxtst-dev libgtk-3-dev

    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    export PATH="$PATH:/path/to/depot_tools"

    mkdir webrtc && cd webrtc
    fetch --nohooks webrtc
    cd src

    # Pin to the branch we build against
    git ls-remote https://chromium.googlesource.com/external/webrtc --heads branch-heads/7151
    git checkout -b local_7151 refs/remotes/branch-heads/7151
    gclient sync

    # Sysroots: install one per target CPU you plan to build for
    build/linux/sysroot_scripts/install-sysroot.py --arch=amd64
    build/linux/sysroot_scripts/install-sysroot.py --arch=arm64

    gn gen out/Default

`gclient sync` pulls in all of WebRTC's third-party dependencies (abseil, libyuv, libvpx, boringssl, etc.) — this step can take a long time and a lot of disk space (tens of GB) on first run.

## 5. Drop de_camera's sources into the WebRTC tree

    cp -R src/* <webrtc-checkout>/src/examples/de_camera/
    cp BUILD.gn <webrtc-checkout>/src/examples/BUILD.gn

You only need to redo this step after changing `de_camera` source files — you do **not** need to re-run `gclient sync` or the sysroot installs again.

### Automating the workflow

If you build often, it's worth wrapping the steps above in your own scripts:

- a **sync** step that copies this repo's `src/` (and `BUILD.gn`) into the WebRTC checkout's `examples/de_camera` (and `examples/BUILD.gn`),
- a **build** step that runs `ninja -C out/<target>` for whichever target(s) you need (optionally installing the matching sysroot first),
- a **collect** step that copies the resulting `de_camera` binary out of `out/<target>/de_camera` into a local binaries folder of your choice, and
- a **deploy** step that copies the collected binary plus `de_camera.config.module.json` / `template.json` to the target device, e.g. via `scp`/`rsync` over SSH keys.

These are plain wrappers around the commands in this document — none of the underlying `gn`/`ninja` steps change.

## 6. gn gen targets and ninja build

Run these from `webrtc/src`. Each `gn gen out/<name> --args='...'` only needs to be run once per output directory — after that, re-running `ninja -C out/<name>` picks up source changes incrementally.

### X86 (desktop / dev machine)

    build/linux/sysroot_scripts/install-sysroot.py --arch=amd64

    # debug
    gn gen out/m137_x86_d --args='is_debug=true use_system_libjpeg=false treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="x64" is_clang=true'
    ninja -C out/m137_x86_d

    # release
    gn gen out/m137_x86_r --args='is_debug=false use_system_libjpeg=false treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="x64" is_clang=true'
    ninja -C out/m137_x86_r

### ARM64 (RPi-5, RPi-4, RPi-Zero 2 W — same binary works on all three)

    build/linux/sysroot_scripts/install-sysroot.py --arch=arm64

    # debug
    gn gen out/rpi-5 --args='is_debug=true use_system_libjpeg=true treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="arm64" is_clang=true'
    ninja -C out/rpi-5

    # release
    gn gen out/rpi-5_r --args='is_debug=false use_system_libjpeg=true treat_warnings_as_errors=false rtc_include_tests=false rtc_use_h264=true rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="arm64" is_clang=true'
    ninja -C out/rpi-5_r

Notes on the args that differ between platforms:

- `use_system_libjpeg`: `false` on x86 (statically links WebRTC's own libjpeg), `true` on ARM (uses the system `libjpeg-dev`).
- `target_cpu`: `"x64"` vs `"arm64"` — this, together with the matching `install-sysroot.py --arch=`, is what cross-compiles for RPi from an x86 dev machine.
- `is_debug`: `true` for a debug build (asserts, no optimization, easier to debug with gdb), `false` for release.

The compiled binary lands at `webrtc/src/out/<target>/de_camera`.

## 7. Deploying to a Raspberry Pi

After building the ARM release binary, copy it plus its config files to the device, e.g.:

    scp out/rpi-5_r/de_camera de_camera.config.module.json template.json \
        pi@<device-host>:/home/pi/drone_engage_binary/de_camera/

Prefer SSH keys over password auth for anything beyond a local test Pi.

## Legacy standalone build (deprecated)

Before the project moved to building inside the WebRTC examples tree, `de_camera` could be compiled standalone against a prebuilt static `libwebrtc.a`:

- WebRTC was built from the older branch `branch-heads/7103` on **Ubuntu 18.04**.
- The resulting `libwebrtc.a` and headers were placed under `lib/webrtc-local/lib/{Debug,Release}` and `lib/webrtc-local/include`.
- The root [`Makefile`](../Makefile) (`make debug` / `make release`) compiled `de_camera`'s own sources with `clang++` and linked them against that static library.

This path is **no longer maintained** — the headers/libs under `lib/webrtc-local/` may be stale relative to current `src/` sources. Use the gn/ninja workflow in this document instead. The Makefile is kept in the repo for reference only.

## See also

- [README.md](../README.md) — quick-reference version of this guide.
- [`scripts/README`](../scripts/README) and [`scripts/compile_webrtc/`](../scripts/compile_webrtc) — the raw `gn gen` command reference these instructions are based on.
