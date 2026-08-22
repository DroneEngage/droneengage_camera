# AGENTS.md — drone_engage_camera_2025

DroneEngage Camera module (`de_camera`). Streams video from multiple V4L2 cameras
to the WebClient using native WebRTC. Built as part of the WebRTC source tree
using `gn`/`ninja`, for both X86 (desktop) and ARM64 (RPi-5/4/Zero 2 W) targets.

See `README.md` and `wiki/` for the full project walkthrough. This file is the
quick reference for agents working in this repo.

## Build

**This project is NOT compiled in-place.** It is compiled via the scripts in
[`local/`](local), which copy `src/` into a checked-out WebRTC source tree,
run `ninja` there, then copy the resulting `de_camera` binaries back into this
repo. The `local/` directory is gitignored and machine-specific (paths are
hard-coded to the developer's WebRTC checkout), so it is not committed.

### One-shot build

    cd local
    ./sh_do_all.sh

This runs the full pipeline in order:

1. `sh_update_code.sh` — copies `src/*` into
   `webrtc/src/examples/de_camera/`.
2. `sh_buildme.sh` — installs sysroots and runs `ninja` for both AMD64
   (`out/m137_x86_r`) and ARM64 (`out/rpi-5_r`) targets.
3. `sh_copy_binaries.sh` — copies the built `de_camera` binaries back into
   `compiled_bin/` (x86) and `compiled_rpi_bin/` (RPi).

### Individual steps

    cd local
    ./sh_update_code.sh          # sync src/ -> WebRTC examples dir
    ./sh_buildme.sh              # build both targets
    ./sh_buildme.sh amd          # build AMD64 only
    ./sh_buildme.sh arm          # build ARM64 (RPi) only
    ./sh_copy_binaries.sh        # collect binaries into compiled_bin/ and compiled_rpi_bin/

### Prerequisites

- A WebRTC M137 (`branch-heads/7151`) checkout with `depot_tools` on `PATH`.
  The `local/` scripts expect it at the hard-coded path
  `/home/mhefny/TDisk_6T/Work/webrtc_2025/webrtc/src` — adjust the paths at the
  top of each script for a different machine.
- `amd64` and `arm64` sysroots installed via
  `build/linux/sysroot_scripts/install-sysroot.py` (the build script re-runs
  this, but the `gn gen` for each output dir must already exist — see
  `README.md` §4 for the `gn gen` invocations).
- System deps: `nlohmann-json3-dev`, `libjsoncpp-dev`, `libjpeg-dev`,
  `libx11-dev`, `libexpat1-dev`, `clang`, `libc++-dev` (see `README.md` §1).

### Output binaries

- `compiled_bin/de_camera` — x86_64 release binary.
- `compiled_rpi_bin/de_camera` — ARM64 (RPi) release binary.

Deploy to a device with plain `scp`/`rsync` of the binary plus its config files
(`de_camera.config.module.json`, `template.json`).

## Configuration

Camera/media behavior lives in `de_camera.config.module.json`. See
`template.json` for the full schema with defaults and descriptions, and
`wiki/configuration.md` for the field reference. Note: some JSON files in this
project contain C-style comments — leave them as-is; do not strip them to
satisfy linters.

`template.json` is an array of named groups sent to the GCS config UI (see
`andruav_webclient_react/src/components/jsc_config_generator.jsx`). The UI
shows a dropdown to pick one group at a time; clicking Apply sends only that
group's output, which is shallow-merged into the module config by
`de_common::CConfigFile::updateJSON` (top-level keys only). Because of this,
camera selection and streaming/media settings are split into separate
top-level objects (`camera` and `streaming`) so editing one group does not
overwrite the other.

The groups are: **Communication** (`module_id`, `s2s_udp_*`,
`use_unix_socket`), **Camera** (selection only — scan range or camera list),
**Streaming** (capture/stream/record resolution and fps), **Media Settings**
(storage, image format, recording bitrate/encoder), and **ICE Servers**
(WebRTC STUN/TURN). Some fields are optional and not present in the shipped
`de_camera.config.module.json` — they have code-level defaults (e.g.
`video_recording_bitrate_kbps` = 2000, `video_recording_hw_encoder` = true,
`s2s_udp_packet_size` = auto-detect (8192 for localhost, 1472 for remote), `use_unix_socket` = false).

### Camera vs Streaming split

- **`camera`** — which cameras to open. Two mutually exclusive styles:
  - **Scan range** — `camera_start_index` / `camera_end_index` scan
    `/dev/video<start..end>` and open every device that exists. Recommended
    for plug-and-play auto-discovery.
  - **Camera list** — an explicit `camera_list` array of
    `{ name, device_num | device_name }` entries, giving each camera a custom
    name that shows up in the WebClient. `device_num` maps to `/dev/videoX`;
    `device_name` matches a virtual video driver name or webcam name.
- **`streaming`** — common resolution/framerate settings that apply to all
  cameras regardless of selection style: `capture_width/height/fps`,
  `video_recording_fps`, `stream_max_width/height`,
  `gcs_image_small_width/height`.

**Backward compatibility:** the code falls back to reading the media fields
from `camera` (old flat style) if `streaming` is not present. If no `camera`
object is present at all, the legacy top-level `camera_start_index` /
`camera_end_index` fields are honored (deprecated, defaults to scanning
0–999).

## Test / virtual cameras

`scripts/` contains helper scripts that feed synthetic test patterns into a
virtual V4L2 device so `de_camera` can be exercised without a real camera:

- `video_clock.sh [device_num]` — `testsrc2` clock pattern (default
  `/dev/video6`).
- `video_colors.sh [device_num]` — `rgbtestsrc` color bars (default
  `/dev/video0`).
- `video_clock_others.sh` — legacy multi-device `ffmpeg` reference commands.

## Code style

Follow the existing coding style, formatting, and naming conventions found
throughout the DroneEngage project files. Do not add or remove comments unless
asked. Keep change summaries concise and high-level.
