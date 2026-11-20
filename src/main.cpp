#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Touch Screen Pins for CYD (Cheap Yellow Display)[span_0](start_span)[span_0](end_span)
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
  tft.setRotation(1); // Landscape mode[span_1](start_span)[span_1](end_span)
  tft.fillScreen(TFT_BLACK);

  // Initialize Touch SPI bus[span_2](start_span)[span_2](end_span)
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touch.begin();
  touch.setRotation(1);

  // Draw UI Header
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("TOUCH CALIBRATOR");

  // Draw Test Buttons
  tft.drawRect(20, 150, 120, 50, TFT_BLUE);
  tft.drawString("BUTTON 1", 30, 165);

  tft.drawRect(180, 150, 120, 50, TFT_RED);
  tft.drawString("BUTTON 1", 190, 165); // or BUTTON 2
}

void loop() {
  if (touch.tirqTouched() && touch.touched()) {
    TS_Point p = touch.getPoint();
    
    // Clear previous coordinate text area on screen
    tft.fillRect(10, 45, 300, 50, TFT_BLACK);
    
    // Display Raw Touch Coordinates directly on the screen
    tft.setTextSize(1);
    tft.setCursor(10, 45);
    tft.printf("RAW X: %d  Y: %d", p.x, p.y);

    // Map raw touch coordinates to screen pixels (Landscape 320x240)[span_3](start_span)[span_3](end_span)
    int touchX = map(p.x, 200, 3700, 0, 320);
    int touchY = map(p.y, 350, 3800, 0, 240);

    tft.setCursor(10, 65);
    tft.printf("Mapped X: %d  Y: %d", touchX, touchY);

    // Check Button Presses
    tft.setTextSize(2);
    if (touchX >= 20 && touchX <= 140 && touchY >= 150 && touchY <= 200) {
      tft.fillRect(10, 100, 300, 30, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.println("Clicked BUTTON 1!");
    } else if (touchX >= 180 && touchX <= 300 && touchY >= 150 && touchY <= 200) {
      tft.fillRect(10, 100, 300, 30, TFT_BLACK);
      tft.setCursor(10, 100);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.println("Clicked BUTTON 2!");
    }
    
    delay(150); // Debounce delay
  }
}
