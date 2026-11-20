#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include <SPIFFS.h>

// Touch Screen Pins for CYD (Cheap Yellow Display)
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

void setup() {
  Serial.begin(115200);
  
  // Initialize Display
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);

  // Initialize Touch with explicit SPI pins
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touch.begin();
  touch.setRotation(1); // Match display rotation

  // Draw UI Header
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("CYD MUSIC OS");

  // Draw Buttons
  tft.drawRect(20, 180, 80, 40, TFT_BLUE);
  tft.drawString("PREV", 35, 192);

  tft.drawRect(120, 180, 80, 40, TFT_GREEN);
  tft.drawString("PLAY", 135, 192);

  tft.drawRect(220, 180, 80, 40, TFT_RED);
  tft.drawString("NEXT", 235, 192);
}

void loop() {
  if (touch.tirqTouched() && touch.touched()) {
    TS_Point p = touch.getPoint();
    
    // Map raw touch coordinates to screen pixels (Landscape 320x240)
    int touchX = map(p.x, 200, 3700, 0, 320);
    int touchY = map(p.y, 350, 3800, 0, 240);

    // Check if PLAY button is pressed (X: 120-200, Y: 180-220)
    if (touchX >= 120 && touchX <= 200 && touchY >= 180 && touchY <= 220) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString("PLAY PRESSED", 20, 80);
    }
    
    delay(200); // Debounce
  }
}
