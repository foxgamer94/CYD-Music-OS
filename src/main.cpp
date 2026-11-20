#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// Target: common Sunton ESP32-2432S028 / CYD, 2.8in ILI9341 + XPT2046.
// Verify the exact board revision before flashing. See README.md.

static constexpr int SD_CS = 5;
static constexpr int TOUCH_CS = 33;
static constexpr int TOUCH_IRQ = 36;
static constexpr int TOUCH_SCK = 25;
static constexpr int TOUCH_MISO = 39;
static constexpr int TOUCH_MOSI = 32;
static constexpr int SPEAKER_DAC_GPIO = 26; // board's onboard amp input
static constexpr int BACKLIGHT = 21;

// XPT2046 is wired to its own non-default SPI pin set on this CYD revision.
SPIClass touchSPI(VSPI);
TFT_eSPI tft;
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

AudioGeneratorMP3 *decoder = nullptr;
AudioFileSourceSD *audioFile = nullptr;
AudioOutputI2S *audioOut = nullptr;

String tracks[80];
int trackCount = 0;
int currentTrack = -1;
bool sdReady = false;
bool paused = false;
bool btMode = false; // Bluetooth routing is a Phase 2 feature, not active in this starter build.
uint8_t volumeLevel = 70;

const uint16_t BG = TFT_BLACK;
const uint16_t PANEL = 0x18E3;
const uint16_t CYAN = 0x05FF;
const uint16_t WHITE = TFT_WHITE;
const uint16_t MUTED = 0x9CF3;
const uint16_t GREEN = 0x07E0;
const uint16_t RED = 0xF800;

int16_t touchMinX = 250, touchMaxX = 3800;
int16_t touchMinY = 250, touchMaxY = 3800;

void stopPlayback() {
  if (decoder) {
    if (decoder->isRunning()) decoder->stop();
    delete decoder;
    decoder = nullptr;
  }
  if (audioFile) {
    audioFile->close();
    delete audioFile;
    audioFile = nullptr;
  }
  if (audioOut) {
    delete audioOut;
    audioOut = nullptr;
  }
  paused = false;
}

void scanMusic() {
  trackCount = 0;
  if (!sdReady) return;
  File dir = SD.open("/");
  if (!dir) return;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory() && trackCount < 80) {
      String name = String(entry.name());
      name.toLowerCase();
      if (name.endsWith(".mp3")) {
        if (!name.startsWith("/")) name = "/" + name;
        tracks[trackCount++] = name;
      }
    }
    entry.close();
  }
  dir.close();
}

void drawButton(int x, int y, int w, int h, const char *label, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 9, color);
  tft.drawRoundRect(x, y, w, h, 9, WHITE);
  tft.setTextColor(WHITE, color);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, x + w/2, y + h/2, 2);
}

String shortName(String path) {
  int slash = path.lastIndexOf('/');
  if (slash >= 0) path = path.substring(slash + 1);
  if (path.length() > 27) path = path.substring(0, 24) + "...";
  return path;
}

void drawUI() {
  tft.fillScreen(BG);
  tft.fillRect(0, 0, 320, 34, PANEL);
  tft.setTextColor(CYAN, PANEL);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("CYD MUSIC OS", 10, 8, 4);
  tft.setTextColor(sdReady ? GREEN : RED, PANEL);
  tft.drawRightString(sdReady ? "SD READY" : "NO SD", 310, 12, 2);

  tft.fillRoundRect(10, 45, 300, 93, 12, PANEL);
  tft.setTextColor(MUTED, PANEL);
  tft.drawString("NOW PLAYING", 22, 55, 2);
  tft.setTextColor(WHITE, PANEL);
  String title = currentTrack >= 0 ? shortName(tracks[currentTrack]) : "Choose an MP3 from microSD";
  tft.drawCentreString(title, 160, 83, 2);
  tft.setTextColor(CYAN, PANEL);
  tft.drawCentreString(btMode ? "OUTPUT: BLUETOOTH (not enabled yet)" : "OUTPUT: ONBOARD SPEAKER", 160, 113, 1);

  drawButton(12, 151, 88, 42, "PREV", 0x3186);
  drawButton(116, 151, 88, 42, paused || !decoder ? "PLAY" : "PAUSE", 0x03A0);
  drawButton(220, 151, 88, 42, "NEXT", 0x3186);

  drawButton(12, 204, 140, 27, "MUSIC LIBRARY", 0x2128);
  drawButton(168, 204, 140, 27, "OUTPUT: SPEAKER", 0x2128);

  tft.setTextColor(MUTED, BG);
  tft.setTextDatum(BL_DATUM);
  tft.drawString("Volume " + String(volumeLevel) + "%  |  MP3 / microSD", 10, 239, 1);
}

void startTrack(int index) {
  if (!sdReady || trackCount == 0) return;
  if (index < 0) index = trackCount - 1;
  if (index >= trackCount) index = 0;
  stopPlayback();
  currentTrack = index;

  audioFile = new AudioFileSourceSD(tracks[currentTrack].c_str());
  audioOut = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC);
  audioOut->SetOutputModeMono(true);
  audioOut->SetGain((float)volumeLevel / 100.0f);
  decoder = new AudioGeneratorMP3();

  if (!decoder->begin(audioFile, audioOut)) {
    stopPlayback();
    Serial.println("MP3 decoder failed to start");
  }
  drawUI();
}

bool readTouch(int &x, int &y) {
  if (!touch.touched()) return false;
  TS_Point p = touch.getPoint();
  // Initial generic calibration. If taps are offset, tune these four values.
  int rawX = map(p.x, touchMinX, touchMaxX, 0, 320);
  int rawY = map(p.y, touchMinY, touchMaxY, 0, 240);
  x = constrain(rawX, 0, 319);
  y = constrain(rawY, 0, 239);
  return true;
}

void showLibrary() {
  tft.fillScreen(BG);
  tft.fillRect(0, 0, 320, 30, PANEL);
  tft.setTextColor(CYAN, PANEL);
  tft.drawString("MUSIC LIBRARY", 10, 7, 2);
  tft.setTextColor(MUTED, BG);
  tft.drawRightString("Tap a track | BACK", 310, 9, 1);
  if (!sdReady) {
    tft.setTextColor(RED, BG);
    tft.drawCentreString("Insert FAT32 microSD card", 160, 110, 2);
    return;
  }
  if (!trackCount) {
    tft.drawCentreString("No MP3 files found in SD root", 160, 110, 2);
    return;
  }
  for (int i = 0; i < min(trackCount, 8); i++) {
    int y = 38 + i * 24;
    tft.fillRoundRect(8, y, 304, 21, 5, i == currentTrack ? 0x03A0 : PANEL);
    tft.setTextColor(WHITE, i == currentTrack ? 0x03A0 : PANEL);
    tft.drawString(String(i + 1) + ". " + shortName(tracks[i]), 14, y + 4, 1);
  }
  tft.fillRoundRect(8, 234, 304, 4, 2, PANEL);
}

bool libraryScreen = false;
void handleTouch() {
  static uint32_t lastTap = 0;
  int x, y;
  if (!readTouch(x, y) || millis() - lastTap < 250) return;
  lastTap = millis();

  if (libraryScreen) {
    if (y < 32 || y > 220) {
      libraryScreen = false;
      drawUI();
      return;
    }
    int idx = (y - 38) / 24;
    if (idx >= 0 && idx < min(trackCount, 8)) {
      startTrack(idx);
      libraryScreen = false;
      drawUI();
    }
    return;
  }

  if (y >= 151 && y <= 193) {
    if (x < 105) startTrack(currentTrack - 1);
    else if (x < 215) {
      if (decoder && decoder->isRunning()) {
        decoder->stop();
        paused = true;
      } else if (paused && audioFile && audioOut) {
        // Decoder restart is safer than assuming pause/resume state survives stop.
        startTrack(currentTrack);
      } else if (trackCount) startTrack(currentTrack < 0 ? 0 : currentTrack);
      drawUI();
    } else startTrack(currentTrack + 1);
  } else if (y >= 204 && y <= 235) {
    if (x < 160) {
      libraryScreen = true;
      showLibrary();
    } else {
      // Output selector is deliberately locked to speaker until BT source routing
      // is implemented and tested in Phase 2.
      btMode = false;
      drawUI();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  tft.init();
  tft.setRotation(1); // landscape 320x240
  tft.fillScreen(BG);

  touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin();
  touch.setRotation(1);

  SPI.begin(18, 19, 23, SD_CS);
  sdReady = SD.begin(SD_CS, SPI, 20000000);
  if (sdReady) scanMusic();

  drawUI();
  Serial.printf("CYD Music OS starter | SD=%s | tracks=%d\n", sdReady ? "OK" : "FAIL", trackCount);
}

void loop() {
  if (!libraryScreen && decoder && decoder->isRunning()) {
    if (!decoder->loop()) {
      decoder->stop();
      drawUI();
    }
  }
  handleTouch();
  delay(1);
}
