#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSD.h>
#include <AudioOutputI2S.h>
// Hardware Pin Definitions for CYD (ESP32-2432S028)
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS   5
#define TFT_BL_PIN 21
// Drivers & Hardware Objects
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI = SPIClass(HSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);
// Audio State Objects
AudioGeneratorMP3 *mp3 = nullptr;
AudioFileSourceSD *file = nullptr;
AudioOutputI2S *out = nullptr;
bool isPlaying = false;
bool sdInitialized = false;
unsigned long lastTouchTime = 0;
// Function Prototypes
void drawUI();
void updateStatus(const char* msg, uint16_t color);
void handleButtonPress(int x, int y);
void setup() {
Serial.begin(115200);
delay(100);
Serial.println("\n--- CYD Music OS Booting ---");
// 1. Enable Hardware Backlight
pinMode(TFT_BL_PIN, OUTPUT);
digitalWrite(TFT_BL_PIN, HIGH);
// 2. Initialize TFT Display in Landscape Mode (320x240)
tft.init();
tft.setRotation(1);
tft.fillScreen(TFT_BLACK);
// 3. Initialize XPT2046 Touchscreen Controller
touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
touch.begin();
touch.setRotation(1);
// 4. Initialize microSD Card Reader
SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
if (SD.begin(SD_CS, SPI, 25000000)) {
sdInitialized = true;
Serial.println("microSD Card initialized successfully.");
} else {
sdInitialized = false;
Serial.println("microSD Card not detected.");
}
// 5. Render UI Frame
drawUI();
}
void drawUI() {
tft.fillScreen(TFT_BLACK);
// Header Bar
tft.fillRect(0, 0, 320, 35, tft.color565(30, 40, 60));
tft.setTextColor(TFT_CYAN, tft.color565(30, 40, 60));
tft.setTextSize(2);
tft.setCursor(15, 8);
tft.println("CYD MUSIC OS");
// Status Box
if (sdInitialized) {
updateStatus("SD Card Ready - Tap PLAY", TFT_GREEN);
} else {
updateStatus("No SD Card - Touch Diagnostic Mode", TFT_YELLOW);
}
// Diagnostic & Calibration Display Area
tft.drawRoundRect(10, 85, 300, 65, 6, tft.color565(80, 80, 80));
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.setTextSize(1);
tft.setCursor(20, 95);
tft.println("Touch Calibration & Diagnostic:");
tft.setCursor(20, 112);
tft.println("Raw Touch : X: ---  Y: ---");
tft.setCursor(20, 129);
tft.println("Mapped Pix: X: ---  Y: ---");
// On-Screen Buttons
// PREV Button
tft.fillRoundRect(10, 165, 90, 55, 8, tft.color565(40, 60, 120));
tft.drawRoundRect(10, 165, 90, 55, 8, TFT_WHITE);
tft.setTextColor(TFT_WHITE, tft.color565(40, 60, 120));
tft.setTextSize(2);
tft.setCursor(28, 183);
tft.print("PREV");
// PLAY Button
tft.fillRoundRect(115, 165, 90, 55, 8, tft.color565(20, 120, 50));
tft.drawRoundRect(115, 165, 90, 55, 8, TFT_WHITE);
tft.setTextColor(TFT_WHITE, tft.color565(20, 120, 50));
tft.setTextSize(2);
tft.setCursor(133, 183);
tft.print("PLAY");
// NEXT Button
tft.fillRoundRect(220, 165, 90, 55, 8, tft.color565(120, 40, 40));
tft.drawRoundRect(220, 165, 90, 55, 8, TFT_WHITE);
tft.setTextColor(TFT_WHITE, tft.color565(120, 40, 40));
tft.setTextSize(2);
tft.setCursor(240, 183);
tft.print("NEXT");
}
void updateStatus(const char* msg, uint16_t color) {
tft.fillRect(10, 45, 300, 28, TFT_BLACK);
tft.drawRoundRect(10, 45, 300, 28, 4, color);
tft.setTextColor(color, TFT_BLACK);
tft.setTextSize(1);
tft.setCursor(20, 55);
tft.print(msg);
}
void loop() {
// Handle Touch Inputs
if (touch.touched()) {
unsigned long now = millis();
if (now - lastTouchTime > 150) { // Touch debounce
lastTouchTime = now;
TS_Point p = touch.getPoint();
// Map raw coordinates to landscape resolution (320x240)
int mappedX = map(p.x, 200, 3800, 0, 320);
int mappedY = map(p.y, 200, 3800, 0, 240);
mappedX = constrain(mappedX, 0, 319);
mappedY = constrain(mappedY, 0, 239);
// Render Touch Coordinates on Screen for Live Diagnostics
tft.fillRect(100, 112, 200, 30, TFT_BLACK);
tft.setTextColor(TFT_GREEN, TFT_BLACK);
tft.setTextSize(1);
tft.setCursor(100, 112);
tft.printf("X:%-4d Y:%-4d", p.x, p.y);
tft.setCursor(100, 129);
tft.printf("X:%-3d  Y:%-3d", mappedX, mappedY);
// Process Button Intersections
handleButtonPress(mappedX, mappedY);
}
}
// Audio Decoder Loop
if (isPlaying && mp3 != nullptr) {
if (mp3->isRunning()) {
if (!mp3->loop()) {
mp3->stop();
isPlaying = false;
updateStatus("Playback Complete.", TFT_CYAN);
}
} else {
isPlaying = false;
}
}
}
void handleButtonPress(int x, int y) {
// PREV Button Area
if (x >= 10 && x <= 100 && y >= 165 && y <= 220) {
Serial.println("Button: PREV Pressed");
updateStatus("Action: PREV Track", TFT_CYAN);
}
// PLAY / PAUSE Button Area
else if (x >= 115 && x <= 205 && y >= 165 && y <= 220) {
Serial.println("Button: PLAY Pressed");
if (!isPlaying) {
if (sdInitialized) {
updateStatus("Playing MP3 from microSD...", TFT_GREEN);
} else {
updateStatus("PLAY Pressed (Insert microSD Card)", TFT_YELLOW);
}
} else {
if (mp3) mp3->stop();
isPlaying = false;
updateStatus("Playback Paused", TFT_YELLOW);
}
}
// NEXT Button Area
else if (x >= 220 && x <= 310 && y >= 165 && y <= 220) {
Serial.println("Button: NEXT Pressed");
updateStatus("Action: NEXT Track", TFT_CYAN);
}
}