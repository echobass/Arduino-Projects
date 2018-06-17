#include <Adafruit_NeoPixel.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

/* *************** CONFIGURATION ************************************ */

// Behaviour configuration
#define DELAY_MS 25 // Set above 0 to delay some milliseconds, messes with spectrum measurements for some reason?
#define LEVEL_REDUCTION 120 // Flat reduction for all audio band levels, accounts for background and internal noise
#define SENSITIVITY 50 // 0 No Sensitivity (Off) -> 50 Ideal/Center - 100 High

// Logging configuration
#define LOGGING_ENABLED false // Logs to Serial 9600, slows things down though
#define TICKS_BETWEEN_LOGS 200 // Set above 0 to only log every so ticks

/* *************** LCD CONFIGURATION ************************************ */

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define LCD_RED 0x1
#define LCD_YELLOW 0x3
#define LCD_GREEN 0x2
#define LCD_TEAL 0x6
#define LCD_BLUE 0x4
#define LCD_VIOLET 0x5
#define LCD_WHITE 0x7

#define LCD_CHARACTERS 16
#define LCD_ROWS 2

/* *************** LED CONFIGURATION ************************************ */

#define LED_PIN 6
#define LED_COUNT 144
#define LED_GAMMA NEO_GRB

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, LED_GAMMA + NEO_KHZ800);

/* *************** UNIQUE SPECTRUM CONFIGURATION ************************************ */

#define PIN_SPECTRUM_LEFT 0 // MIC CONNECTED TO LEFT
#define PIN_SPECTRUM_RIGHT 1
#define PIN_SPECTRUM_SOURCE PIN_SPECTRUM_LEFT

/* *************** SPECTRUM COMMON CONFIGURATION ************************************ */

// Spectrum specs
#define BANDS 7 // Audio spectrum bands
#define MAX_LEVEL 1023.0 // Max readable value for audio (analog) levels
#define IDEAL_SENSITIVITY 50 // A centerpoint for calculating sensitivity

// Spectrum pins
// For spectrum analyzer shield, these three pins are used.
// You can move pinds 4 and 5, but you must cut the trace on the shield and re-route from the 2 jumpers
#define PIN_SPECTRUM_RESET 5
#define PIN_SPECTRUM_STROBE 4

// Trailing averages, each calculated with the Counts below
#define TRAILING_BUCKETS 3
#define SHORT_AVERAGE 0
#define MED_AVERAGE 1
#define LONG_AVERAGE 2
int trailingAverageCounts[TRAILING_BUCKETS] = {10, 100, 10000}; // # of ticks used to calculate averages


/* *************** GLOBAL VARS ************************************ */

uint8_t LAST_BUTTONS = 0;

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

// Init calculated constants
int LEVEL_COEFFICIENT = (float)IDEAL_SENSITIVITY / (float)SENSITIVITY;
int LEVEL_ADJUST = ((float)LEVEL_REDUCTION) * LEVEL_COEFFICIENT;

// Init runtime vars
int logCounter = 0;

/* *************** SPECTRUM ************************************ */

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
  //calculateTrailingAverages(trailingAverages, spectrumAverage);
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
  
}

void setupSpectrum() {
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

void updateSpectrum() {
  readSpectrum();
}

/* *************** COLOURS ************************************ */

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

/* *************** DRAWING ************************************ */

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void setupStrip() {
  strip.begin();
  strip.setBrightness(15); //adjust brightness here
  strip.show(); // Initialize all pixels to 'off'
}

void drawStrip() {
  // Some example procedures showing how to display to the pixels:
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  //rainbow(1);
  rainbowCycle(1);
}

/* *************** MODES ************************************ */

void cycleMode() {
  
}

void toggleSoundActive() {
  
}

/* *************** LCD INTERFACE ************************************ */

void quickButtonPress() {
  cycleMode();
}

void longButtonPress() {
  toggleSoundActive();
}

void handleButtons(uint8_t buttons) {
  if (buttons & BUTTON_UP) { // Up Button
    if (LAST_BUTTONS & BUTTON_UP) { // Hold
    } else { // New Press
      quickButtonPress();
    }
  }
  if (buttons & BUTTON_DOWN) { // Down Button
    if (LAST_BUTTONS & BUTTON_DOWN) { // Hold
    } else { // New Press
      quickButtonPress();
    }
  }
  if (buttons & BUTTON_LEFT) { // Left Button
    if (LAST_BUTTONS & BUTTON_LEFT) { // Hold
    } else { // New Press
      quickButtonPress();
    }
  }
  if (buttons & BUTTON_RIGHT) { // Right Button
    if (LAST_BUTTONS & BUTTON_RIGHT) { // Hold
    } else { // New Press
      quickButtonPress();
    }
  }
  if (buttons & BUTTON_SELECT) { // Select Button
    if (LAST_BUTTONS & BUTTON_SELECT) { // Hold
    } else { // New Press
      quickButtonPress();
    }
  }
}

void setupLCD() {
  lcd.begin(LCD_CHARACTERS, LCD_ROWS);
  
  lcd.setBacklight(LCD_WHITE);
  lcd.setCursor(0, 0);
  lcd.print("  Don't");
  lcd.setCursor(0, 1);
  lcd.print("        Worry!");
  delay(2000);
  
  lcd.clear();
  lcd.setBacklight(LCD_VIOLET);
}

void drawMode() {
  lcd.setCursor(0, 0);
  lcd.print("Mode");
  lcd.setCursor(0, 1);
  lcd.print("...");
}

void drawLCD() {
  drawMode();
}

void handleLCD() {
  drawLCD();
}

/* *************** RUNTIME ************************************ */

void loop() {
  handleLCD();
  
  updateSpectrum();

  drawStrip();
}

void setup() {
  Serial.begin(9600);

  setupLCD();
  setupSpectrum();
  setupStrip();

  updateSpectrum();

  drawLCD();
  drawStrip();
}
