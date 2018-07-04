/* https://learn.sparkfun.com/tutorials/spectrum-shield-hookup-guide */
/* https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library */
/* https://learn.adafruit.com/multi-tasking-the-arduino-part-3 */
/* https://github.com/adafruit/Adafruit_NeoMatrix/blob/master/Adafruit_NeoMatrix.h */
/* https://github.com/adafruit/Adafruit_NeoMatrix/blob/master/gamma.h */
/* https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.h */

/*
 * TODO:
 * - Pixel draw animation (incrementally manipulate LEDs as they move up and down)
 * - Pixel fade animation (LEDs dim over time after lighting up)
 * - 24bit colour
 * - Read and understand Sparkfun's advanced example
 * - Configurable settings for colours/effect/etc.
 * - Beat detection
 * - Audio isolation
 * - Random flash/sparkle effect
 */

/*
 * BUGS?
 * - Orange band drawing red?
 *    Centerline
 *    Colour per band
 *    Always draw center = false
 */

/*
 * MIC ONLY!
 * - Currently using a shield that is mic only
 * - Mic feeds to LEFT
 */

#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <MD_REncoder.h>
#include <FlashStorage.h>
#include <Button.h>

/* *************** CONFIGURATION ************************************ */

// Behaviour configuration
#define DRAW_DELAY_MS 25 // Set above 0 to delay some milliseconds, messes with spectrum measurements for some reason?

// Logging configuration
#define LOGGING_ENABLED true // Logs to Serial 9600, slows things down though
#define TICKS_BETWEEN_LOGS 200 // Set above 0 to only log every so ticks

// Display configuration
#define MIC_OVERRIDE false // Set to true to use mic instead of audio in
#define BRIGHTNESS 25 // Set the overall brightness; Low values will affect colours

// Matrix specs
#define COLUMNS 8 // Width
#define ROWS 5 // Height

// Data pin for LEDs
#define PIN_NEOMATRIX 6

// Sensitivity Adjustment
#define PIN_SENSITIVITY 2

// Rotary Encoder
#define PIN_ENCODER_A 8
#define PIN_ENCODER_B 9
#define PIN_ENCODER_SWITCH 10 // Unused

MD_REncoder rotaryEncoder = MD_REncoder(PIN_ENCODER_A, PIN_ENCODER_B);
Button rotaryClick(PIN_ENCODER_SWITCH); // Connect your button between pin 2 and GND


/* *************** GLOBAL VARS ************************************ */

// Declare the neomatrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(COLUMNS, ROWS, PIN_NEOMATRIX,
                                      NEO_MATRIX_TOP        + NEO_MATRIX_LEFT +
                                      NEO_MATRIX_ROWS       + NEO_MATRIX_PROGRESSIVE,
                                      NEO_GRBW              + NEO_KHZ800);

// Init runtime vars
int logCounter = 0;

/* *************** SPECTRUM ************************************ */

// Trailing averages, each calculated with the Counts below
#define TRAILING_BUCKETS 2
#define SHORT_AVERAGE 0
#define MED_AVERAGE 1
//#define LONG_AVERAGE 2
//int trailingAverageCounts[TRAILING_BUCKETS] = {10, 100, 10000}; // # of ticks used to calculate averages
int trailingAverageCounts[TRAILING_BUCKETS] = {10, 100}; // # of ticks used to calculate averages

// Spectrum specs
#define BANDS 7 // Audio spectrum bands
#define MAX_LEVEL 1023.0 // Max readable value for audio (analog) levels

// Spectrum pins
// For spectrum analyzer shield, these three pins are used.
// You can move pinds 4 and 5, but you must cut the trace on the shield and re-route from the 2 jumpers
#define PIN_SPECTRUM_RESET 5
#define PIN_SPECTRUM_STROBE 4
#define PIN_SPECTRUM_LEFT 0
#define PIN_SPECTRUM_RIGHT 1
int PIN_SPECTRUM_SOURCE = MIC_OVERRIDE ? PIN_SPECTRUM_RIGHT : PIN_SPECTRUM_LEFT;

// Spectrum analyzer frame values will be kept here
int spectrum[BANDS];

// Calculated values for each frame
int spectrumMin;
int spectrumMax;
int spectrumSum;
float spectrumAverage;

// Trailing averages
float trailingSpectrumLevels[BANDS][TRAILING_BUCKETS];
//float trailingMinimums[TRAILING_BUCKETS];
float trailingMaximums[TRAILING_BUCKETS];
//float trailingSums[TRAILING_BUCKETS];
//float trailingAverages[TRAILING_BUCKETS];

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
  //calculateTrailingAverages(trailingSums, spectrumSum);
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

void initAnalyzer() {
  pinMode(PIN_SPECTRUM_RESET, OUTPUT);
  pinMode(PIN_SPECTRUM_STROBE, OUTPUT);
  pinMode(PIN_SPECTRUM_SOURCE, INPUT);
 
  // Create an initial state for our pins
  digitalWrite (PIN_SPECTRUM_RESET,  LOW);
  digitalWrite (PIN_SPECTRUM_STROBE, LOW);
  delay        (1);
 
  // Reset the MSGEQ7 as per the datasheet timing diagram
  digitalWrite (PIN_SPECTRUM_RESET,  HIGH);
  delay        (1);
  digitalWrite (PIN_SPECTRUM_RESET,  LOW);
  digitalWrite (PIN_SPECTRUM_STROBE, HIGH);
  delay        (1);

  /*
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
  */
  // Reading the analyzer now will read the lowest frequency.
}

/* *************** COLUMN CALCULATIONS ************************************ */

int getColumnLevel(int column) {  
  int level;
  
  // Column 0: Average
  // Column 1-7: Aaudio frequency bands
  
  if (column == 0) {
    level = spectrumAverage;
    //level = max(level, trailingAverages[SHORT_AVERAGE]); // Smooth against trailing average
    level = max(spectrumMax, trailingMaximums[SHORT_AVERAGE]); // Smooth against trailing average
  } else {
    int band = column-1;
    level = spectrum[band];
    //level = max(level, trailingSpectrumLevels[band][SHORT_AVERAGE]); // Smooth against trailing average
    level = max(level, trailingWeightedSpectrumLevels[band][SHORT_AVERAGE]); // Smooth against trailing average
  }

  return level;
}

float calculateColumnRatio(int column, int level) {
  // Read the sensitvity and calculate an adjustment
  // Flat reduction for all audio band levels, accounts for background and internal noise
  int sensitivityAdjustment = analogRead(PIN_SENSITIVITY);
  //sensitivityAdjustment = 64 + ((sensitivityAdjustment / MAX_LEVEL) * 256); // Baseline 64, Normalize to roughly 0-256
  // For science: don't normalize (yet)

  // Adjust for baseline noise etc
  //  Keeps the board dark when silent
  //  Unfortunately, forces music to be loud
  //  Need a way to scale so that lower volumes still work...
  int adjustedLevel = level - sensitivityAdjustment;

  // Reality is... Baseline and internal noise is indistinguishable from low volume signals
  //  Can't automagically account for one without ruining the other
  //   Therefore... Sensitivity adjustment?

  // Our starting factor, the current spectrumMax
  float factor = (float) spectrumMax;
  
  // Reduce to account for noise & sensitivity
  //  Where to do this??  Shouldn't trailingMaximums...
  //   Apply to maximums?
  //   Apply to raw readings?
  factor = factor - sensitivityAdjustment; // Don't curr

  // Smooth against trailing averages
  factor = max(factor, min(trailingMaximums[SHORT_AVERAGE], trailingMaximums[MED_AVERAGE]));

  // Show absolute volume with column 0
  if (column == 0) {
    factor = MAX_LEVEL;
  }

  // Calculate the ratio of LEDs to draw!
  float ratio = ((float) adjustedLevel) / factor;

  if (false && LOGGING_ENABLED && logCounter == TICKS_BETWEEN_LOGS) {
    Serial.print("c: ");
    Serial.print(column);
    Serial.print(", l: ");
    Serial.print(level);
    Serial.print(", al: ");
    Serial.print(adjustedLevel);
    Serial.print(", r: ");
    Serial.print(ratio);
    Serial.print("\n");
  }

  return ratio;
}

/* *************** COLOURS ************************************ */

// Tuned for 100 brightness
const uint16_t column_colours[] = {
  matrix.Color(255, 255, 255), // White
  matrix.Color(255, 0, 0), // Red
  matrix.Color(255, 102, 34), // Orange
  matrix.Color(255, 218, 33), // Yellow
  matrix.Color(51, 221, 0), // Green
  matrix.Color(17, 180, 180), // Teal
  matrix.Color(17, 70, 255), // Blue
  matrix.Color(180, 0, 255) // Violet
};

// Tuned for 100 brightness
const uint16_t classic_row_colours[] = {
  matrix.Color(51, 221, 0), // Green
  matrix.Color(51, 221, 0), // Green
  matrix.Color(255, 218, 33), // Yellow
  matrix.Color(255, 102, 34), // Orange
  matrix.Color(255, 0, 0), // Red
};

// Tuned for 100 brightness
const uint16_t cool1_row_colours[] = {
  matrix.Color(255, 0, 0), // Red
  matrix.Color(255, 218, 33), // Yellow
  matrix.Color(51, 221, 0), // Green
  matrix.Color(17, 70, 255), // Blue
  matrix.Color(180, 0, 255), // Violet
};

// Tuned for 100 brightness
const uint16_t coolx_row_colours[] = {
  matrix.Color(255, 0, 0), // Red
  matrix.Color(255, 218, 33), // Yellow
  matrix.Color(51, 221, 0), // Green
  matrix.Color(17, 70, 255), // Blue
  matrix.Color(180, 0, 255), // Violet
};

// Tuned for 100 brightness
const uint16_t cool_row_colours[] = {
  matrix.Color(180, 0, 255), // Violet
  matrix.Color(180, 0, 255), // Violet
  matrix.Color(17, 70, 255), // Blue
  matrix.Color(255, 0, 0), // Red
  matrix.Color(255, 0, 0), // Red
};

// Colours manually tuned for 2/4 Brightness
//  less than 2/4 causes gamma issues
const uint16_t column_colours_dim[] = {
  matrix.Color(100, 100, 100), // White
  matrix.Color(100, 0, 0), // Red
  matrix.Color(120, 40, 20), // Orange
  matrix.Color(90, 80, 40), // Yellow
  matrix.Color(10, 70, 0), // Green
  matrix.Color(00, 80, 80), // Teal
  matrix.Color(10, 0, 140), // Blue
  matrix.Color(60, 0, 80) // Violet
};

// Colours manually tuned for 2/4 Brightness
//  less than 2/4 causes gamma issues
const uint16_t classic_row_colours_dim[] = {
  matrix.Color(10, 70, 0), // Green
  matrix.Color(10, 70, 0), // Green
  matrix.Color(90, 80, 40), // Yellow
  matrix.Color(120, 40, 20), // Orange
  matrix.Color(100, 0, 0), // Red
};

static const uint8_t PROGMEM
  gamma5[] = {
    0x00,0x01,0x02,0x03,0x05,0x07,0x09,0x0b,
    0x0e,0x11,0x14,0x18,0x1d,0x22,0x28,0x2e,
    0x36,0x3d,0x46,0x4f,0x59,0x64,0x6f,0x7c,
    0x89,0x97,0xa6,0xb6,0xc7,0xd9,0xeb,0xff },
  gamma6[] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x08,
    0x09,0x0a,0x0b,0x0d,0x0e,0x10,0x12,0x13,
    0x15,0x17,0x19,0x1b,0x1d,0x20,0x22,0x25,
    0x27,0x2a,0x2d,0x30,0x33,0x37,0x3a,0x3e,
    0x41,0x45,0x49,0x4d,0x52,0x56,0x5b,0x5f,
    0x64,0x69,0x6e,0x74,0x79,0x7f,0x85,0x8b,
    0x91,0x97,0x9d,0xa4,0xab,0xb2,0xb9,0xc0,
    0xc7,0xcf,0xd6,0xde,0xe6,0xee,0xf7,0xff };

// Returns the Red component of a 16-bit color
uint8_t Red(uint16_t color) {
    return (color >> 8) & 0xF8;
}
// Returns the Green component of a 16-bit color
uint8_t Green(uint16_t color) {
    return (color >> 3) & 0xFC;
}
// Returns the Blue component of a 16-bit color
uint8_t Blue(uint16_t color) {
    return (color << 3);
}

// Return color (dimmed by 75% ???)
uint16_t DimColor(uint16_t colour, int degree=1) {
  // Get the components
  uint8_t red = Red(colour);
  uint8_t green = Green(colour);
  uint8_t blue = Blue(colour);

  int dimmedColour = matrix.Color(Red(colour) >> degree, Green(colour) >> degree, Blue(colour) >> degree);
  //return dimmedColour;
  
  // Only dim if there's room to
  if (Red(dimmedColour) < 0x10 || Green(dimmedColour) < 0x10 || Blue(dimmedColour) < 0x10) {
    return colour;
  } else {
    return dimmedColour;
  }

  /*
  if (red > 0x10) red = red >> degree;
  if (green > 0x10) green = green >> degree;
  if (blue > 0x10) blue = blue >> degree;
  return matrix.Color(red, green, blue);
   */
  
  // Otherwise return the original colour
  //return matrix.Color(Red(colour) >> degree, Green(colour) >> degree, Blue(colour) >> degree);
  //return colour;
}

/* *************** DRAWING ************************************ */

/*
int getColour(float ratio, int num_lights, int column, int y, int dim=0) {
  int colour;
  // Set desired colourscheme to true/1

  if (0) // Solid colur
    colour = column_colours[4];
  if (0) // Color wheel
    colour = column_colours[(int)(7*ratio)];
  if (1) // Unique colours to each column
    colour = column_colours[column];
  if (0) // Classic
    colour = classic_row_colours[ROWS-1-y];
  if (0) // Cool
    colour = cool_row_colours[ROWS-1-y];

  // Dimming effects
  if (0) // Bright maximums
    if (y != 0 && y != 4) // Only for centerlined
      colour = DimColor(colour);
  if (0) // Bright tips
    if (y != (3-num_lights)) // Only for centerlined
      colour = DimColor(colour);

  // Apply dim
  for (int i = 0; i < dim; i++) {
    colour = DimColor(colour);
  }

  return colour;
}

void drawColumn(int column, float ratio, int dim=0, bool onlyPeaks=false, bool alwaysCenter=false) {
  int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
  int colour;
  // Set desired effect(s) to true/1

  if (1) { // Centerlined
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      colour = getColour(ratio, num_lights, column, ((y+0.5)/(float)rows)*ROWS, dim);

      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (y == 0) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (y == 1) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (y == 2) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
      if (onlyPeaks) break;
    }
    if (alwaysCenter) { // Always draw center row
      if (1) { // Only if it didn't move
        if (num_lights < 1) {
          colour = getColour(ratio, num_lights, column, center, dim);
          matrix.drawPixel(column, center, colour);
        }
      }
    }
  } else { // Bottom-up
    for (int y = (ROWS - num_lights); y < ROWS; y++) {
      colour = getColour(ratio, num_lights, column, y, dim);
      matrix.drawPixel(column, y, colour);
      if (onlyPeaks) break;
    }
    if (alwaysCenter) { // Always draw bottom row
      colour = getColour(ratio, num_lights, column, 4, dim);
      matrix.drawPixel(column, 4, colour);
    }
  }
}

void drawSpectrum() {
  matrix.fillScreen(0); // Blank the screen

  for (int column = 0; column < COLUMNS; column++) {
    // Select the appropriate level for this column
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    
    //drawColumn(column, ratio, 0, true, true);
    drawColumn(column, ratio, 0, false, false);
  }

  matrix.show();
}
 */

/* *************** VU ************************************ */

void drawRainbowVu() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    for (int y = (ROWS - num_lights); y < ROWS; y++) {
      int colour = column_colours[column];
      matrix.drawPixel(column, y, colour);
    }
  }
  matrix.show();
}

void drawClassicVu() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    for (int y = (ROWS - num_lights); y < ROWS; y++) {
      int colour = classic_row_colours[ROWS-1-y];
      matrix.drawPixel(column, y, colour);
    }
  }
  matrix.show();
}

void drawCoolVu() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    for (int y = (ROWS - num_lights); y < ROWS; y++) {
      int colour = cool_row_colours[ROWS-1-y];
      matrix.drawPixel(column, y, colour);
    }
  }
  matrix.show();
}

/* *************** OSC ************************************ */

void drawColorOsc(int colour) {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
    }
  }
  matrix.show();
}

void drawRainbowOsc() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = column_colours[column];
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
    }
  }
  matrix.show();
}

void drawClassicOsc() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = classic_row_colours[ROWS-1-(int)(((y+0.5)/(float)rows)*ROWS)]; // Don't curr
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
    }
  }
  matrix.show();
}

void drawCoolOsc() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = cool_row_colours[ROWS-1-(int)(((y+0.5)/(float)rows)*ROWS)]; // Don't curr
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
    }
  }
  matrix.show();
}

/* *************** OSC TIPS ************************************ */

void drawColorOscTips(int colour) {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
      
      break; // Only draw peaks
    }
    
    if (num_lights < 1) { // Always draw center row
      matrix.drawPixel(column, center, colour);
    }
  }
  matrix.show();
}

void drawRainbowOscTips() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = column_colours[column];
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
      
      break; // Only draw peaks
    }
    
    if (num_lights < 1) { // Always draw center row
      int colour = column_colours[column];
      matrix.drawPixel(column, center, colour);
    }
  }
  matrix.show();
}

void drawClassicOscTips() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = classic_row_colours[ROWS-1-(int)(((y+0.5)/(float)rows)*ROWS)]; // Don't curr
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
      
      break; // Only draw peaks
    }
    
    if (num_lights < 1) { // Always draw center row
      int colour = classic_row_colours[ROWS-1-(int)((center/(float)rows)*ROWS)]; // Don't curr
      matrix.drawPixel(column, center, colour);
    }
  }
  matrix.show();
}

void drawCoolOscTips() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = cool_row_colours[ROWS-1-(int)(((y+0.5)/(float)rows)*ROWS)]; // Don't curr
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
      
      break; // Only draw peaks
    }
    
    if (num_lights < 1) { // Always draw center row
      int colour = cool_row_colours[ROWS-1-(int)((center/(float)rows)*ROWS)]; // Don't curr
      matrix.drawPixel(column, center, colour);
    }
  }
  matrix.show();
}

/* *************** PRISMATIC ************************************ */

int interpolateColor(int primary, int secondary, float sigma = 0.5) {
  float alpha = sigma * 2.0;
  float beta = (1 - sigma) * 2.0;
  return matrix.Color(
    (int)(Red(primary)*alpha + Red(secondary)*beta) / 2,
    (int)(Green(primary)*alpha + Green(secondary)*beta) / 2,
    (int)(Blue(primary)*alpha + Blue(secondary)*beta) / 2
  );
}

void drawPrismOsc() {
  matrix.fillScreen(0);
  for (int column = 0; column < COLUMNS; column++) {
    int level = getColumnLevel(column);
    float ratio = calculateColumnRatio(column, level);
    int num_lights = (ROWS * ratio) + 0.5; // Add 0.5 for lazy integer rounding-up
    int center = ROWS / 2;
    int rows = center + 1;
    num_lights = (rows * ratio) + 0.5;
    for (int y = (rows - num_lights); y < rows; y++) {
      int colour = interpolateColor(column_colours[column], column_colours[0], min(ratio+0.25, 1.0));
      
      // Apply gradient dim
      int dimLevels = 4;
      int steps = rows * ratio * dimLevels;
      if (num_lights == 1) {
        colour = DimColor(colour, (steps % dimLevels));
      } else if (num_lights == 2) {
        if (steps < 8) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      } else if (num_lights == 3) {
        if (steps < 4) {
          colour = DimColor(colour, (steps % dimLevels));
        }
      }
      
      matrix.drawPixel(column, y, colour);
      if (y != center) { // Draw mirrored LED if off-center
        int mirror_y = ROWS - 1 - y;
        matrix.drawPixel(column, mirror_y, colour);
      }
    }
    
    if (num_lights < 1) { // Always draw center row
      int colour = DimColor(column_colours[0]);
      matrix.drawPixel(column, center, colour);
    }
  }
  matrix.show();
}

/* *************** BEAT DRIVEN ************************************ */

int currentBeatColor = 0;
int lastBeatMillis = 0; // 1000ms (60pm) to 3000ms (180bpm)

void drawRainbowOscBeat() {
  unsigned long now = millis();
  if ((now > lastBeatMillis+1000) && (trailingMaximums[SHORT_AVERAGE] > trailingMaximums[MED_AVERAGE]) && (trailingMaximums[SHORT_AVERAGE] > spectrumMax)) {
    currentBeatColor = (currentBeatColor + 1) % 8;
    lastBeatMillis = now;
  }
  drawColorOsc(column_colours[currentBeatColor]);
}

void drawRainbowOscCrazy() {
  if ((trailingMaximums[SHORT_AVERAGE] > trailingMaximums[MED_AVERAGE]) || (trailingMaximums[SHORT_AVERAGE] > spectrumMax)) {
    currentBeatColor = (currentBeatColor + 1) % 8;
  }
  drawColorOsc(column_colours[currentBeatColor]);
}

void drawRainbowOscTipsBeat() {
  unsigned long now = millis();
  if ((now > lastBeatMillis+1000) && (trailingMaximums[SHORT_AVERAGE] > trailingMaximums[MED_AVERAGE]) && (trailingMaximums[SHORT_AVERAGE] > spectrumMax)) {
    currentBeatColor = (currentBeatColor + 1) % 8;
    lastBeatMillis = now;
  }
  drawColorOscTips(column_colours[currentBeatColor]);
}

void drawRainbowOscTipsCrazy() {
  unsigned long now = millis();
  if ((trailingMaximums[SHORT_AVERAGE] > trailingMaximums[MED_AVERAGE]) || (trailingMaximums[SHORT_AVERAGE] > spectrumMax)) {
    currentBeatColor = (currentBeatColor + 1) % 8;
  }
  if (currentBeatColor == 0) {
    drawColorOscTips(DimColor(column_colours[currentBeatColor]));
  } else {
    drawColorOscTips(column_colours[currentBeatColor]);
  }
}

/* *************** MODES ************************************ */

#define PRISM_OSC             1

#define RAINBOW_VU            2
#define COOL_VU               3
#define CLASSIC_VU            4

#define RAINBOW_OSC_BEAT        5
#define RAINBOW_OSC_CRAZY       6
#define RAINBOW_OSC_TIPS_BEAT   7
#define RAINBOW_OSC_TIPS_CRAZY  8

#define WHITE_OSC             11
#define RED_OSC               12
#define ORANGE_OSC            13
#define YELLOW_OSC            14
#define GREEN_OSC             15
#define TEAL_OSC              16
#define BLUE_OSC              17
#define PURPLE_OSC            18

#define RAINBOW_OSC           21
#define CLASSIC_OSC           22
#define COOL_OSC              23

#define WHITE_OSC_TIPS        31
#define RED_OSC_TIPS          32
#define ORANGE_OSC_TIPS       33
#define YELLOW_OSC_TIPS       34
#define GREEN_OSC_TIPS        35
#define TEAL_OSC_TIPS         36
#define BLUE_OSC_TIPS         37
#define PURPLE_OSC_TIPS       38

#define RAINBOW_OSC_TIPS      41
#define CLASSIC_OSC_TIPS      42
#define COOL_OSC_TIPS         43

int orderedPresets[] = {
  PRISM_OSC,
  
  WHITE_OSC,
  RED_OSC,
  ORANGE_OSC,
  YELLOW_OSC,
  GREEN_OSC,
  TEAL_OSC,
  BLUE_OSC,
  PURPLE_OSC,

  RAINBOW_OSC,
  CLASSIC_OSC,
  COOL_OSC,
  
  RAINBOW_OSC_BEAT,
  RAINBOW_OSC_CRAZY,

  WHITE_OSC_TIPS,
  RED_OSC_TIPS,
  ORANGE_OSC_TIPS,
  YELLOW_OSC_TIPS,
  GREEN_OSC_TIPS,
  TEAL_OSC_TIPS,
  BLUE_OSC_TIPS,
  PURPLE_OSC_TIPS,

  RAINBOW_OSC_TIPS,
  CLASSIC_OSC_TIPS,
  COOL_OSC_TIPS,
  
  RAINBOW_OSC_TIPS_BEAT,
  RAINBOW_OSC_TIPS_CRAZY,

  RAINBOW_VU,
  COOL_VU,
  CLASSIC_VU
};
int numPresets = sizeof(orderedPresets) / sizeof(int);

uint8_t defaultPresetIndex = 0;
uint8_t startingPresetIndex = defaultPresetIndex;

uint8_t currentPresetIndex = startingPresetIndex;
int currentPreset = orderedPresets[startingPresetIndex];

int millisBeforeSave = 5000; // 5 seconds
unsigned long lastChangeTime = millis();
boolean modeSaved = true;

FlashStorage(lastModeFlash, uint8_t);

void startPresetByIndex(int index) {
  currentPresetIndex = index;
  currentPreset = orderedPresets[index];
  
  modeSaved = false;
  lastChangeTime = millis();
}

void cyclePreset(boolean forward = true) {
  int newPresetIndex;
  if (forward) {
    newPresetIndex = (numPresets + currentPresetIndex + 1) % numPresets;
    Serial.println(newPresetIndex);
    Serial.println("Forward");
  } else {
    newPresetIndex = (numPresets + currentPresetIndex - 1) % numPresets;
    Serial.println(newPresetIndex);
    Serial.println("Backwards");
  }
  startPresetByIndex(newPresetIndex);
}

void drawByPreset(int preset) {
  switch (preset) {
    case PRISM_OSC:
      drawPrismOsc();
      break;

    case RAINBOW_OSC_BEAT:
      drawRainbowOscBeat();
      break;
    case RAINBOW_OSC_CRAZY:
      drawRainbowOscCrazy();
      break;
    case RAINBOW_OSC_TIPS_BEAT:
      drawRainbowOscTipsBeat();
      break;
    case RAINBOW_OSC_TIPS_CRAZY:
      drawRainbowOscTipsCrazy();
      break;

    case WHITE_OSC:
      drawColorOsc(column_colours[0]);
      break;
    case RED_OSC:
      drawColorOsc(column_colours[1]);
      break;
    case ORANGE_OSC:
      drawColorOsc(column_colours[2]);
      break;
    case YELLOW_OSC:
      drawColorOsc(column_colours[3]);
      break;
    case GREEN_OSC:
      drawColorOsc(column_colours[4]);
      break;
    case TEAL_OSC:
      drawColorOsc(column_colours[5]);
      break;
    case BLUE_OSC:
      drawColorOsc(column_colours[6]);
      break;
    case PURPLE_OSC:
      drawColorOsc(column_colours[7]);
      break;
      
    case RAINBOW_OSC:
      drawRainbowOsc();
      break;
    case CLASSIC_OSC:
      drawClassicOsc();
      break;
    case COOL_OSC:
      drawCoolOsc();
      break;
      
    case WHITE_OSC_TIPS:
      drawColorOscTips(DimColor(column_colours[0]));
      break;
    case RED_OSC_TIPS:
      drawColorOscTips(column_colours[1]);
      break;
    case ORANGE_OSC_TIPS:
      drawColorOscTips(column_colours[2]);
      break;
    case YELLOW_OSC_TIPS:
      drawColorOscTips(column_colours[3]);
      break;
    case GREEN_OSC_TIPS:
      drawColorOscTips(column_colours[4]);
      break;
    case TEAL_OSC_TIPS:
      drawColorOscTips(column_colours[5]);
      break;
    case BLUE_OSC_TIPS:
      drawColorOscTips(column_colours[6]);
      break;
    case PURPLE_OSC_TIPS:
      drawColorOscTips(column_colours[7]);
      break;
      
    case RAINBOW_OSC_TIPS:
      drawRainbowOscTips();
      break;
    case CLASSIC_OSC_TIPS:
      drawClassicOscTips();
      break;
    case COOL_OSC_TIPS:
      drawCoolOscTips();
      break;
    
    case RAINBOW_VU:
      drawRainbowVu();
      break;
    case CLASSIC_VU:
      drawClassicVu();
      break;
    case COOL_VU:
      drawCoolVu();
      break;
      
    default:
      // Draw nothing
      break;
  }
}

/* *************** RUNTIME ************************************ */

unsigned long lastDraw = millis() + 2000; // Force initial draw delay for 1500ms

void handleEncoder() {
  uint8_t encoderTurn = rotaryEncoder.read();
  if (encoderTurn) {
    cyclePreset(encoderTurn == DIR_CCW);
  }
}

void loop() {
  unsigned long now = millis();

  if (rotaryClick.pressed()) {
    startPresetByIndex(defaultPresetIndex);
  }
  
  if (!modeSaved && (now > (millisBeforeSave + lastChangeTime))) {
    lastModeFlash.write(currentPresetIndex);
    modeSaved = true;
  }
  
  readSpectrum();
  if (LOGGING_ENABLED && logCounter == TICKS_BETWEEN_LOGS) {
    logSpectrum();
  }
    
  
  if (now > (DRAW_DELAY_MS + lastDraw)) {
    drawByPreset(currentPreset);
    lastDraw = now;
  }

  if (LOGGING_ENABLED && logCounter == TICKS_BETWEEN_LOGS) {
    Serial.println("----");
    logCounter = 0;
  }
  logCounter++; // Don't curr
}

// The setup function runs once when you press reset or power the board
void setup() {
  // Init serial logging
  if (LOGGING_ENABLED) {
    Serial.begin(9600);
  }

  // Rotary Encoder
  rotaryEncoder.begin();
  attachInterrupt(PIN_ENCODER_A, handleEncoder, CHANGE); 
  attachInterrupt(PIN_ENCODER_B, handleEncoder, CHANGE);

  // Sensitivity Pot
  pinMode(PIN_SENSITIVITY, INPUT);
  
  rotaryClick.begin();
  
  // Init the spectrum analyzer
  initAnalyzer();

  // Init the draw mode
  uint8_t savedModeIndex = lastModeFlash.read();
  if (savedModeIndex > 0 && savedModeIndex < numPresets) {
    startPresetByIndex(savedModeIndex);
    modeSaved = true; // Prevent re-saving
  }

  // Init the matrix
  matrix.begin();
  matrix.setBrightness(BRIGHTNESS);
  for (int c = 0; c < COLUMNS; c++) {
    matrix.drawPixel(c, 2, DimColor(c));
    matrix.show();
  }
  for (int p = 0; p < COLUMNS; p++) {
    delay(20);
    for (int c = 0; c < COLUMNS; c++) {
      delay(20);
      if (c+p+1 >= COLUMNS) {
        matrix.drawPixel(c, 2, DimColor(column_colours[0]));
      } else {
        matrix.drawPixel(c, 2, column_colours[p]);
      }
      matrix.show();
    }
  }
  matrix.drawPixel(0, 2, DimColor(column_colours[0]));
}

