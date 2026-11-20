# CYD Music OS — starter firmware project

Target: common Sunton ESP32-2432S028 / CYD, ESP32-WROOM-32, 2.8-inch ILI9341,
XPT2046 resistive touch, microSD, and the onboard speaker amplifier.

## What this starter build currently implements

- Landscape touchscreen UI
- microSD initialization and MP3 scan (MP3 files in the card's root directory)
- MP3 decoding with ESP8266Audio
- Audio output through the board's onboard amplifier / speaker connector (GPIO26 DAC)
- Basic previous / play / next and library selection

## Not implemented yet (clearly marked future work)

- Bluetooth headphone audio source routing and output switching
- Spotify account authentication / remote playback controls
- Internet radio, playlists, album art, and metadata-rich browsing

This project does **not** stream Spotify tracks directly. Spotify playback is protected
and the ESP32 is not a supported native Spotify client. A later companion/remote-control
implementation can use Spotify's supported Web API where account and Premium
requirements permit, while the phone/computer remains the actual Spotify playback device.

## Hardware notes

The board photos show the back marking `ESP32_2432S028`. The common 2432S028 pin map is:

| Function | GPIO |
|---|---:|
| ILI9341 TFT: SCLK / MOSI / MISO | 14 / 13 / 12 |
| TFT CS / DC / backlight | 15 / 2 / 21 |
| XPT2046 touch: CLK / MOSI / MISO | 25 / 32 / 39 |
| Touch CS / IRQ | 33 / 36 |
| microSD: SCK / MISO / MOSI / CS | 18 / 19 / 23 / 5 |
| onboard speaker amplifier input | 26 |

The 2-pin JST speaker socket is an amplified speaker output. Connect a suitable
speaker to that socket; do not connect headphones directly to it. The ESP32 internal
DAC feeds the onboard amplifier via GPIO26. The DAC is not a headphone driver.

Board revisions can differ. Confirm your exact display/touch variant and speaker
hardware before using this pin map. Some CYD variants use a different display driver
or touch controller.

## Build on a computer

1. Install VS Code and PlatformIO IDE, or install PlatformIO Core.
2. Open this folder as a PlatformIO project.
3. Let PlatformIO install the dependencies.
4. Build: `pio run`
5. Connect the board with a data-capable USB cable.
6. Upload: `pio run -t upload`
7. Serial monitor: `pio device monitor -b 115200`

The first successful build produces firmware under:
`.pio/build/cyd_esp32_2432s028/firmware.bin`

PlatformIO's default partition table is selected in `platformio.ini`. This starter
project does not provide a precompiled binary because it has not been compiled
against your specific board revision in this environment.

## microSD preparation

- Format the card as FAT32.
- Copy a few MP3 files into the card's top-level/root folder.
- Insert it before powering the CYD.
- Use short, simple filenames for the first test.

## Touch calibration

Touch raw coordinates vary by panel. If taps do not line up, edit
`touchMinX`, `touchMaxX`, `touchMinY`, and `touchMaxY` in `src/main.cpp`.
Use the serial monitor and a small raw-touch diagnostic if needed. The initial
values are generic, not guaranteed calibrated for your individual panel.

## Known starter limitations

- The library screen displays the first eight root-level MP3 files.
- The pause button currently stops the decoder; pressing play restarts the current
  track from the beginning. A true pause/resume implementation is a follow-up.
- The UI and audio path need a real hardware test before calling this firmware stable.
- Bluetooth and Spotify UI controls are reserved for Phase 2; they are not active
  in this first starter firmware.
