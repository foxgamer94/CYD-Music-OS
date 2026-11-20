# CYD Music OS roadmap

## Phase 1 — local player (this project)
- [x] Identify common 2432S028 pin map from board markings/photos
- [x] Create PlatformIO starter project
- [x] Add TFT + touch + microSD + MP3 speaker playback path
- [ ] Build with PlatformIO and resolve version-specific compiler issues
- [ ] Flash and calibrate touchscreen on the physical board
- [ ] Test SD cards and speaker level

## Phase 2 — Bluetooth audio output
Implement ESP32 A2DP Source and an audio-output adapter that feeds decoded PCM
samples to the Bluetooth stack. Add pairing UI and route switching. The ESP32's
Bluetooth Classic A2DP source support must be validated against the selected
Arduino-ESP32 core and library versions. Do not enable simultaneous speaker +
Bluetooth output in this design; user selected output switching.

## Phase 3 — Spotify remote controls
Add Wi-Fi setup and a companion OAuth flow for Spotify Web API playback control.
The ESP32 should not store a Spotify password. Use OAuth authorization, token
refresh, and the supported playback endpoints. A Premium account and an active
Spotify Connect playback device may be required for playback-control endpoints.
This is remote control, not direct Spotify audio decoding on the CYD.

## Phase 4 — polish
- ID3 title/artist/album parsing
- Recursive folders and playlists
- Sleep timer, brightness control, progress bar
- Wi-Fi internet radio
- OTA updates with rollback
