// include the library code:
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

/* *************** LCD CONFIGURATION ************************************ */

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

/* *************** BEAT CONFIGURATION ************************************ */

/*
 * TODO
 * - Two modes: Manual & Automatic
 * - Flashing beat indicator
 * - Backlight flash beat indicator
 * - Spectrum display if audio present
 * - Only update bpm on button press
 * - Indicator for reset/hold
 * - Ignore simultaneous clicks?
 */

// Min BPM: 50 (1.2s) ?
// Max BPM: 190 (0.31s) ?

#define RESET_MS 2000
#define BEAT_WINDOW_MS 75

/* *************** GLOBAL VARS ************************************ */

unsigned long NOW = 0;
unsigned long ELAPSED = 0;

unsigned long LAST_BEAT = 0;
unsigned long LAST_BEAT_PRESS = 0;
uint8_t LAST_BUTTONS = 0;

boolean HAVE_BEAT = false;

unsigned int BEAT_TIME = 0;
unsigned long NEXT_BEAT_MILLIS = 0;
unsigned long NEXT_BEAT_MILLIS_LOW = 0;
unsigned long NEXT_BEAT_MILLIS_HIGH = 0;

float BPM = 0.0;
float TRAILING_BPM = 0.0;
int TRAILING_COUNT = 0;

/* *************** DRAWING ************************************ */

void drawBeat() {
  lcd.setCursor(0, 1);
  lcd.print("BEAT");
}

void hideBeat() {
  lcd.setCursor(0, 1);
  lcd.print("    ");
}

void drawReset() {
  lcd.setCursor(10, 0);
  lcd.print(">RESET");
}

void hideReset() {
  lcd.setCursor(10, 0);
  lcd.print("      ");
}

void drawBpm() {
  lcd.setCursor(1, 0);
  lcd.print("     ");
  lcd.setCursor(1, 0);
  if (HAVE_BEAT) {
    lcd.print(TRAILING_BPM, 1);
  } else {
    lcd.print("000.0");
  }
}

void drawMode() {
  lcd.setCursor(0, 1);
  lcd.print("      Manual BPM");
}

/* *************** BEAT DETECTION ************************************ */

void resetBpm() {
  BPM = 0.0;
  TRAILING_BPM = 0.0;
  TRAILING_COUNT = 0;
  HAVE_BEAT = false;
  LAST_BEAT = 0;
  NEXT_BEAT_MILLIS = 0;
}

void updateBeat() {
  if (NOW > NEXT_BEAT_MILLIS_HIGH) {
    LAST_BEAT = NOW;
    NEXT_BEAT_MILLIS = LAST_BEAT + BEAT_TIME;
    NEXT_BEAT_MILLIS_LOW = NEXT_BEAT_MILLIS - BEAT_WINDOW_MS;
    NEXT_BEAT_MILLIS_HIGH = NEXT_BEAT_MILLIS + BEAT_WINDOW_MS;
  }
}

void updateBpm() {
  // handle special cases
  if (LAST_BEAT_PRESS == 0) { // First press on startup
    resetBpm();
    return;
  }
  if (ELAPSED >= RESET_MS) { // First press after reset
    resetBpm();
    return;
  }

  // Update BPM
  BPM = 60.0 / (ELAPSED / 1000.0);

  // Update trailing average stats
  TRAILING_BPM = ((TRAILING_COUNT * TRAILING_BPM) + BPM) / (TRAILING_COUNT + 1);
  TRAILING_COUNT = TRAILING_COUNT + 1;

  // And we go!
  BEAT_TIME = (60.0 / TRAILING_BPM) * 1000;
  HAVE_BEAT = true;
  LAST_BEAT_PRESS = NOW;
  LAST_BEAT = NOW;
}

/* *************** INPUT ************************************ */

void handleButtons(uint8_t buttons) {
  if (buttons & BUTTON_UP) { // Up Button
    if (LAST_BUTTONS & BUTTON_UP) { // Hold
    } else { // New Press
      updateBpm();
      drawBpm();
    }
  }
  if (buttons & BUTTON_DOWN) { // Down Button
    if (LAST_BUTTONS & BUTTON_DOWN) { // Hold
    } else { // New Press
      updateBpm();
      drawBpm();
    }
  }
  if (buttons & BUTTON_LEFT) { // Left Button
    if (LAST_BUTTONS & BUTTON_LEFT) { // Hold
    } else { // New Press
      updateBpm();
      drawBpm();
    }
  }
  if (buttons & BUTTON_RIGHT) { // Right Button
    if (LAST_BUTTONS & BUTTON_RIGHT) { // Hold
    } else { // New Press
      updateBpm();
      drawBpm();
    }
  }
  if (buttons & BUTTON_SELECT) { // Select Button
    if (LAST_BUTTONS & BUTTON_SELECT) { // Hold
    } else { // New Press
      // Toggle between manual & auto
    }
  }
}

/* *************** RUNTIME ************************************ */

void loop() {
  NOW = millis();
  ELAPSED = NOW - LAST_BEAT_PRESS;

  // Handle the buttons
  uint8_t buttons = lcd.readButtons();
  if (buttons) {
    handleButtons(buttons);
  }
  LAST_BUTTONS = buttons;

  // Do things!
  if (HAVE_BEAT) {
    updateBeat();

    // Flash the beat indicator
    if ((NOW > NEXT_BEAT_MILLIS_LOW) && (NOW <= NEXT_BEAT_MILLIS_HIGH)) { // Within range for beat indicator
      drawBeat();
    } else {
      hideBeat();
    }

    // Render the RESET message maybe
    if (ELAPSED >= RESET_MS) {
      drawReset();
    }
  } else { // Reset state
    hideBeat();
    hideReset();
  }
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
  
  drawBpm();
  drawMode();
}
