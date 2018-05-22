// include the library code:
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define CHARACTERS 16
#define ROWS 2

/*
 * TODO
 * - Two modes: Manual & Automatic
 * - Flashing beat indicator
 * - Backlight flash beat indicator
 * - Spectrum display if audio present
 * - Only update bpm on button press
 * - Indicator for reset/hold
 */

// Min BPM: 50 (1.2s) ?
// Max BPM: 190 (0.31s) ?

#define RESET_SECONDS 2

unsigned long lastPress = 0;
uint8_t lastButtons = 0;

float bpm = 0.0;
float trailingAverageBpm = 0.0; // Unimplemented

void clearRow(int row) {
  // Better: Pad the update with spaces
  if (row != 0 && row != 1)
    return;
  lcd.setCursor(0, row);
  lcd.print("     "); // 5 Spaces
}

void drawUpdate() {
  clearRow(0);
  lcd.setCursor(0, 0);
  if (bpm == 0) {
    lcd.print("000.0");
  } else {
    lcd.print(bpm, 1);
  }
}

void updateBpm() {
  unsigned long now = millis();

  if (lastPress == 0) {
    lastPress = now;
    bpm = 0.0;
    return;
  }
  
  unsigned long elapsed = now - lastPress;
 
  if (elapsed >= (RESET_SECONDS * 1000)) {
    lastPress = now;
    bpm = 0.0;
    return;
  }
  
  bpm = 60.0 / (elapsed / 1000.0);
  // TODO: Update trailing average
  lastPress = now;
}

void loop() {
  uint8_t buttons = lcd.readButtons();

  if (buttons) {
    if (buttons & BUTTON_UP) { // Up Button
      if (lastButtons & BUTTON_UP) { // Hold
      } else { // New Press
      }
    }
    if (buttons & BUTTON_DOWN) { // Down Button
      if (lastButtons & BUTTON_DOWN) { // Hold
      } else { // New Press
      }
    }
    if (buttons & BUTTON_LEFT) { // Left Button
      if (lastButtons & BUTTON_LEFT) { // Hold
      } else { // New Press
        updateBpm();
        drawUpdate();
        lastPress = millis();
      }
    }
    if (buttons & BUTTON_RIGHT) { // Right Button
      if (lastButtons & BUTTON_RIGHT) { // Hold
      } else { // New Press
      }
    }
    if (buttons & BUTTON_SELECT) { // Select Button
      if (lastButtons & BUTTON_SELECT) { // Hold
      } else { // New Press
        updateBpm();
        drawUpdate();
        lastPress = millis();
      }
    }
  }

  lastButtons = buttons;
}

void draw() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("             BPM");
  drawUpdate();
}

void setup() {
  Serial.begin(9600);
  
  lcd.begin(CHARACTERS, ROWS);
  
  lcd.setBacklight(WHITE);
  lcd.setCursor(0, 0);
  lcd.print(" Hakuna");
  lcd.setCursor(0, 1);
  lcd.print("        Matata!");
  delay(2000);
  
  lcd.clear();
  lcd.setBacklight(VIOLET);
  draw();
}
