# Changelog

Major changes to `de_camera` — the DroneEngage WebRTC camera streaming module. Newest first.

## [5.10.5] - 2026-09-13

- Local config overrides (`*.local` files) are now applied on top of the module config.
- Fixed the camera scan range.
- Camera-location staleness now uses the monotonic clock.

## [5.10.x] - 2026-09

- Added module health monitoring (`de_module_health.cpp` wired into the build).
- `de_common` submodule updates; raised the memory ceiling.

## [5.8.0] - 2026-08-29

- WebRTC build update.
- Fixed shutdown hang: waiting threads are now unblocked when the module uninitializes.

## [5.7.0] - 2026-08-17

- Split the config into separate `camera` (device selection) and `streaming` (media) objects, so editing one group in the GCS UI does not overwrite the other.

## [5.5.0 / 5.0.0] - 2026-08

- Recording fps is now synced with ffmpeg, and config values are documented as maximums.
- Added configurable capture, streaming, and recording resolutions.
- Added a small still-image mode for the GCS.

## [4.3.0] - 2026-08-13

- Replaced Y4M recording with ffmpeg H.264/MP4 and added hardware-encoder support.
- Added JSON config validation and fixed thread-safety issues.

## [4.1.0 / 4.0.0] - 2026-07

- Added unix-socket support for on-vehicle module traffic.
- Updated ICE configuration, added a camera setup script, and improved camera-list broadcasting.

## [3.12.0] - 2026-02-08

- Reworked camera capture lifecycle and session management with reference counting.
- Fixed duplicate stream requests when two clients share one web connector.

## [3.x earlier] - 2025

- Added a simulated camera for testing and automatic image-folder creation.
- Early releases: WebRTC video streaming to the GCS.
