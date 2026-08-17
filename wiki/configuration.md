# Camera & Media Configuration

`de_camera` is configured through two JSON files:

- **`de_camera.config.module.json`** — the active module config (this is what `de_camera` reads at startup).
- **`template.json`** — the schema/template with defaults and field descriptions, used by the GCS config UI (`jsc_config_generator.jsx`).

This page documents the camera/streaming config blocks and the resolution/framerate semantics introduced/stabilized in **v5.0.0**. For the build/deploy workflow see [webrtc_build_setup.md](webrtc_build_setup.md); for the quick reference see the top-level [README.md](../README.md).

---

## Structure

Camera selection and streaming/media settings are split into two separate top-level objects so the GCS config UI (which merges one group at a time) can edit either without overwriting the other:

- **`camera`** — which cameras to open (selection only).
- **`streaming`** — resolution/framerate settings common to all cameras.

> **Backward compatibility:** the code falls back to reading the media fields from `camera` (old flat style) if `streaming` is not present.

---

## Camera selection (`camera`)

The `camera` object supports two mutually exclusive styles:

### 1. Scan range (recommended)

```json
"camera": {
    "camera_start_index": 0,
    "camera_end_index": 11
}
```

`de_camera` scans `/dev/video<start>` through `/dev/video<end>` and opens every device that exists. Simple and auto-discovers whatever is plugged in.

### 2. Camera list (advanced)

```json
"camera": {
    "camera_list": [
        { "name": "CAM1", "device_num": 1 },
        { "name": "CAM2", "device_name": "DE-CAM2" }
    ]
}
```

Each entry assigns a custom name (shown in the WebClient) to a specific device. `device_num` maps to `/dev/videoX`; `device_name` matches a virtual video driver name or webcam name.

> Backward compatibility: if no `camera` object is present, the old top-level `camera_start_index` / `camera_end_index` fields are honored. This is deprecated and will be removed in a later version.

---

## Streaming settings (`streaming`)

All resolution and framerate fields in the `streaming` object are **maximum / target** values, not exact requirements. The V4L2 driver picks the closest supported mode via `GetBestMatchedCapability`, so a camera whose native resolution is lower than the configured value simply uses its own maximum — you do not need to tailor the config to each camera.

| Field | Default | Description |
| --- | --- | --- |
| `capture_width` | `1024` | Capture width requested from the V4L2 device (closest supported mode is picked). |
| `capture_height` | `720` | Capture height requested from the V4L2 device (closest supported mode is picked). |
| `capture_fps` | `30` | Capture framerate requested from the V4L2 device. |
| `video_recording_fps` | `10` | Recording framerate passed to ffmpeg. `0` = use `capture_fps`. Acts as a max/target — see below. |
| `stream_max_width` | `1280` | WebRTC broadcaster downscale cap. `0` or `-1` = unlimited. |
| `stream_max_height` | `720` | WebRTC broadcaster downscale cap. `0` or `-1` = unlimited. |
| `gcs_image_small_width` | `320` | Target width for the small GCS still image. `0` or `-1` = unlimited (full-res to GCS). |
| `gcs_image_small_height` | `200` | Target height for the small GCS still image. `0` or `-1` = unlimited (full-res to GCS). |

### `video_recording_fps` — max/target, synced with ffmpeg

`video_recording_fps` is the framerate passed to ffmpeg's `-framerate` input flag. It is a **max/target**, not a guarantee:

- If the camera delivers **at least** this many fps, the recorder samples one frame every `1000 / video_recording_fps` ms and ffmpeg writes at the configured rate.
- If the camera delivers **fewer** fps than configured, the sampling interval check passes for every frame and each frame is written as it arrives. ffmpeg's `-framerate` still matches the actual delivery rate closely enough for correct playback speed.
- **Do not** set `video_recording_fps` higher than the camera's actual fps — that would only insert duplicate frames / pad the timeline.
- `0` means "use `capture_fps`".

> **v5.0.0 change:** the recorder now derives its frame-sampling interval directly from `video_recording_fps` (`1000 / rec_fps` ms) so it stays in sync with the `-framerate` passed to ffmpeg. Previously the interval came from a separate timer field, which could desync recorded playback speed from real time when `video_recording_fps` differed from the timer's source.

### `stream_max_width` / `stream_max_height` — downscale cap only

These cap the WebRTC broadcast resolution to protect RPi CPU and uplink bandwidth. Frames that are **already smaller** than the cap are broadcast as-is (no upscale). `0` or `-1` disables the cap entirely.

### `gcs_image_small_width` / `gcs_image_small_height` — GCS thumbnail target

This is the target size for the "small" still image downlinked to the GCS. If the camera's native resolution is already at or below this target, the full-resolution image is sent to the GCS instead of downscaling. The **locally saved** still always keeps the full capture resolution regardless of this setting. `0` or `-1` sends full resolution to the GCS as well.

---

## Top-level media settings

These fields live at the top level of the config (outside `camera` and `streaming`):

| Field | Default | Description |
| --- | --- | --- |
| `media_folder` | `./img` | Folder used to store recorded video and still images. |
| `media_image_png` | `true` | Save still images as PNG (`true`) or BMP (`false`). |
| `send_image_gcs` | `true` | Send captured still images to the GCS. |
| `video_recording_bitrate_kbps` | `2000` | H.264 target bitrate (kbps) passed to ffmpeg for MP4 recording. Optional — not in the shipped config; code default is 2000. |
| `video_recording_hw_encoder` | `true` | Use the Raspberry Pi V4L2 M2M hardware H.264 encoder (`h264_v4l2m2m`) instead of software libx264. Auto-detected from device-tree; set to `false` to force software encoding. Optional — not in the shipped config. |

---

## See also

- [README.md](../README.md) — quick reference and build instructions.
- [webrtc_build_setup.md](webrtc_build_setup.md) — full build/deploy walkthrough.
- [`template.json`](../template.json) — the canonical schema with defaults and descriptions.
