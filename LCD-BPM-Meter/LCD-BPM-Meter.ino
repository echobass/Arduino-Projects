// include the library code:
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

/* *************** MODE CONFIGURATION ************************************ */

// TODO: Inplement SMART mode
#define SMART_MODE 0
#define MANUAL_MODE 1
#define AUDIO_MODE 2
#define MODES 2
int MODE_ORDER[MODES] = {MANUAL_MODE, AUDIO_MODE};

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
 * - Exponential or weighted moving average (probably more realistic)
 */

// Min BPM: 50 (1.2s) ?
// Max BPM: 190 (0.31s) ?

#define RESET_MS 2000
#define BEAT_WINDOW_MS 75

/* *************** SPECTRUM ************************************ */

#define MIC_OVERRIDE false // Set to true to use mic instead of audio in

#define BANDS 7 // Audio spectrum bands

// Spectrum pins
// For spectrum analyzer shield, these three pins are used.
// You can move pinds 4 and 5, but you must cut the trace on the shield and re-route from the 2 jumpers
#define PIN_SPECTRUM_RESET 5
#define PIN_SPECTRUM_STROBE 4
#define PIN_SPECTRUM_LEFT 0
#define PIN_SPECTRUM_RIGHT 1
int PIN_SPECTRUM_SOURCE = MIC_OVERRIDE ? PIN_SPECTRUM_RIGHT : PIN_SPECTRUM_LEFT;

// Trailing averages, each calculated with the Counts below
#define TRAILING_BUCKETS 3
#define SHORT_AVERAGE 0
#define MED_AVERAGE 1
#define LONG_AVERAGE 2
int trailingAverageCounts[TRAILING_BUCKETS] = {10, 100, 10000}; // # of ticks used to calculate averages

// Spectrum analyzer frame values will be kept here
int spectrum[BANDS];

// Calculated values for each frame
int spectrumMin;
int spectrumMax;
int spectrumSum;
float spectrumAverage;

// Trailing averages
float trailingSpectrumLevels[BANDS][TRAILING_BUCKETS];
float trailingMinimums[TRAILING_BUCKETS];
float trailingMaximums[TRAILING_BUCKETS];
float trailingSums[TRAILING_BUCKETS];
float trailingAverages[TRAILING_BUCKETS];

float trailingWeightedSpectrumLevels[BANDS][TRAILING_BUCKETS];

void calculateTrailingAverages(float bucketOfAverages[], float newValue) {
  for (int a = 0; a < TRAILING_BUCKETS; a++) {
    bucketOfAverages[a] = ((bucketOfAverages[a] * (trailingAverageCounts[a]-1)) + newValue) / trailingAverageCounts[a];
  }
}

void calculateWeightedTrailingAverages(float bucketOfWeightedAverages[], float newValue) {
  float alpha;
  for (int a = 0; a < TRAILING_BUCKETS; a++) {
    alpha = 1.0 - ((float)trailingAverageCounts[a] / (float)(trailingAverageCounts[0]+trailingAverageCounts[2]));
    bucketOfWeightedAverages[a] = (alpha * newValue) + ((1.0-alpha) * bucketOfWeightedAverages[a]);
  }
}

// Reads the 7 band analyzer
void readSpectrum() {
  // Reset calculated frame values
  spectrumMin = 0;
  spectrumMax = 0;
  spectrumSum = 0;
  spectrumAverage = 0;

  // band 0 = Lowest Frequencies.
  for (byte band = 0; band < BANDS; band++) {
    // Read twice and take the average by dividing by 2
    spectrum[band] = (analogRead(PIN_SPECTRUM_SOURCE) + analogRead(PIN_SPECTRUM_SOURCE)) >> 1;
    digitalWrite(PIN_SPECTRUM_STROBE, HIGH);
    digitalWrite(PIN_SPECTRUM_STROBE, LOW);

    // Increment the sum for this frame
    spectrumSum = spectrumSum + spectrum[band];

    // Check for min & max
    if (spectrum[band] < spectrumMin) {
      spectrumMin = spectrum[band];
    }
    if (spectrum[band] > spectrumMax) {
      spectrumMax = spectrum[band];
    }

    // Update trailing averages for this band
    calculateTrailingAverages(trailingSpectrumLevels[band], spectrum[band]);

    // Update trailing averages for this band
    calculateWeightedTrailingAverages(trailingWeightedSpectrumLevels[band], spectrum[band]);
  }

  // Calculate average for this frame
  spectrumAverage = spectrumSum / (float) BANDS;

  // Update all calculated trailing averages
  //calculateTrailingAverages(trailingMinimums, spectrumMin);
  calculateTrailingAverages(trailingMaximums, spectrumMax);
  calculateTrailingAverages(trailingSums, spectrumSum);
  calculateTrailingAverages(trailingAverages, spectrumAverage);
}

// Prints the band levels and average to Serial
void logSpectrum() {
  for (byte band = 0; band < BANDS; band++) {
    Serial.print("band: ");
    Serial.print(band);
    Serial.print(", level: ");
    Serial.print(spectrum[band]);
    Serial.print("\n");
  }
  Serial.print("average: ");
  Serial.print(spectrumAverage);
  Serial.print("\n");
}

void resetAnalyzer() {
  pinMode(PIN_SPECTRUM_RESET, OUTPUT);
  pinMode(PIN_SPECTRUM_STROBE, OUTPUT);
  digitalWrite(PIN_SPECTRUM_STROBE, LOW);
  delay(1);
  digitalWrite(PIN_SPECTRUM_STROBE, HIGH);
  delay(1);
  digitalWrite(PIN_SPECTRUM_STROBE, HIGH);
  delay(1);
  digitalWrite(PIN_SPECTRUM_STROBE, LOW);
  delay(1);
  digitalWrite(PIN_SPECTRUM_STROBE, LOW);
  delay(5);
  // Reading the analyzer now will read the lowest frequency.
}

/* *************** GLOBAL VARS ************************************ */

int MODE_INDEX = 0;
int MODE = MODE_ORDER[MODE_INDEX];

unsigned long NOW = 0;
unsigned long ELAPSED = 0;

unsigned long LAST_BEAT = 0;
unsigned long LAST_BEAT_PRESS = 0;
uint8_t LAST_BUTTONS = 0;

unsigned int BEAT_TIME = 0;
unsigned long NEXT_BEAT_LOW = 0;
unsigned long NEXT_BEAT_HIGH = 0;

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
  if (BPM > 0) {
    lcd.print(BPM, 1);
  } else {
    lcd.print("000.0");
  }
}

void drawMode() {
  lcd.setCursor(0, 1);
  if (MODE == SMART_MODE) {
    lcd.print("       Smart BPM");
  } else if (MODE == MANUAL_MODE) {
    lcd.print("      Manual BPM");
  } else if (MODE == AUDIO_MODE) {
    lcd.print("       Audio BPM");
  }
}

/* *************** BEAT DETECTION ************************************ */

void resetBpm() {
  BPM = 0.0;
  TRAILING_BPM = 0.0;
  TRAILING_COUNT = 0;
  LAST_BEAT = 0;
  NEXT_BEAT_LOW = 0;
  NEXT_BEAT_HIGH = 0;
}

void updateBeat() {
  LAST_BEAT += BEAT_TIME;
  NEXT_BEAT_LOW = LAST_BEAT + BEAT_TIME - BEAT_WINDOW_MS;
  NEXT_BEAT_HIGH = LAST_BEAT + BEAT_TIME + BEAT_WINDOW_MS;
}

void updateManualBpm() {
  // handle special cases
  if (LAST_BEAT_PRESS == 0) { // First press on startup
    resetBpm();
    return;
  }
  if (ELAPSED >= RESET_MS) { // First press after reset
    resetBpm();
    return;
  }

  // Calculate this last instant's bpm
  float instant_bpm = 60.0 / (ELAPSED / 1000.0);

  // Update trailing average stats
  TRAILING_BPM = ((TRAILING_COUNT * TRAILING_BPM) + instant_bpm) / (TRAILING_COUNT + 1);
  TRAILING_COUNT = TRAILING_COUNT + 1;

  // And we go!
  BPM = TRAILING_BPM;
  BEAT_TIME = (60.0 / TRAILING_BPM) * 1000;
  LAST_BEAT = NOW;
  NEXT_BEAT_LOW = NOW + BEAT_TIME - BEAT_WINDOW_MS;
  NEXT_BEAT_HIGH = NOW + BEAT_TIME + BEAT_WINDOW_MS;
}

// Min BPM: 50 (1.2s) ?
// Max BPM: 190 (0.31s) ?
void updateAudioBpm() {
  if (spectrum[0] > trailingWeightedSpectrumLevels[0][1] || spectrum[1] > trailingWeightedSpectrumLevels[1][1]) {
    if (LAST_BEAT == 0) {
      LAST_BEAT = NOW;
      return;
    }
    
    int beatElapsed = NOW - LAST_BEAT;
    
    if (beatElapsed < 310 || beatElapsed > 1200) {
      return;
    }
    
    // Calculate this last instant's bpm
    float instant_bpm = 60.0 / ((NOW - LAST_BEAT) / 1000.0);
  
    // Update trailing average stats
    TRAILING_BPM = instant_bpm;
    //TRAILING_BPM = ((TRAILING_COUNT * TRAILING_BPM) + instant_bpm) / (TRAILING_COUNT + 1);
    //TRAILING_COUNT = TRAILING_COUNT + 1;
  
    // And we go!
    BPM = TRAILING_BPM;
    BEAT_TIME = (60.0 / TRAILING_BPM) * 1000;
    LAST_BEAT = NOW;
    NEXT_BEAT_LOW = NOW + BEAT_TIME - BEAT_WINDOW_MS;
    NEXT_BEAT_HIGH = NOW + BEAT_TIME + BEAT_WINDOW_MS;
  }
}

/* *************** INPUT ************************************ */

void handleButtons(uint8_t buttons) {
  if ((MODE == MANUAL_MODE) || (MODE == SMART_MODE)) {
    if (buttons & BUTTON_UP) { // Up Button
      if (LAST_BUTTONS & BUTTON_UP) { // Hold
      } else { // New Press
        updateManualBpm();
        drawBpm();
        LAST_BEAT_PRESS = NOW;
      }
    }
    if (buttons & BUTTON_DOWN) { // Down Button
      if (LAST_BUTTONS & BUTTON_DOWN) { // Hold
      } else { // New Press
        updateManualBpm();
        drawBpm();
        LAST_BEAT_PRESS = NOW;
      }
    }
    if (buttons & BUTTON_LEFT) { // Left Button
      if (LAST_BUTTONS & BUTTON_LEFT) { // Hold
      } else { // New Press
        updateManualBpm();
        drawBpm();
        LAST_BEAT_PRESS = NOW;
      }
    }
    if (buttons & BUTTON_RIGHT) { // Right Button
      if (LAST_BUTTONS & BUTTON_RIGHT) { // Hold
      } else { // New Press
        updateManualBpm();
        drawBpm();
        LAST_BEAT_PRESS = NOW;
      }
    }
  }
  if (buttons & BUTTON_SELECT) { // Select Button
    if (LAST_BUTTONS & BUTTON_SELECT) { // Hold
    } else { // New Press
      MODE_INDEX = (MODE_INDEX + 1) % MODES;
      MODE = MODE_ORDER[MODE_INDEX];
      drawMode();
      resetBpm();
      drawBpm();
      LAST_BEAT = 0;
      LAST_BEAT_PRESS = 0;
    }
  }
}

boolean _resetEnabled() {
  return (MODE == MANUAL_MODE) || (MODE == SMART_MODE);
}

boolean _manualBeatEnabled() {
  return (MODE == MANUAL_MODE) || (MODE == SMART_MODE);
}

boolean _audioBeatEnabled() {
  return (MODE == AUDIO_MODE) || (MODE == SMART_MODE);
}

/* *************** RUNTIME ************************************ */

void loop() {
  NOW = millis();

  // Do BPM stuff
  if (_manualBeatEnabled()) {
    ELAPSED = NOW - LAST_BEAT_PRESS;
  } else if (_audioBeatEnabled()) {
    readSpectrum();
    updateAudioBpm();
    drawBpm();
  }

  // Handle the buttons
  uint8_t buttons = lcd.readButtons();
  if (buttons) {
    handleButtons(buttons);
  }
  LAST_BUTTONS = buttons;

  // Draw things!
  if (BPM > 0) {
    // Flash the beat indicator
    if ((NOW > NEXT_BEAT_LOW) && (NOW <= NEXT_BEAT_HIGH)) {
      drawBeat();
    } else {
      hideBeat();
    }
    
    if (_manualBeatEnabled()) {
      // Update the last beat if we've moved past it
      if (NOW > NEXT_BEAT_HIGH) {
        updateBeat();
      }
    }
      // Render the RESET message maybe
      if (_resetEnabled()) {
        if (ELAPSED >= RESET_MS) {
          drawReset();
        }
      }
    
  } else { // Reset state
    hideBeat();
    if (_resetEnabled()) {
      hideReset();
    }
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

  resetAnalyzer();
  
  lcd.clear();
  lcd.setBacklight(VIOLET);
  
  drawBpm();
  drawMode();
}
