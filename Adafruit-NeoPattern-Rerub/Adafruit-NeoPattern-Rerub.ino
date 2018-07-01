#include <Adafruit_NeoPixel.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

// Dynamic speed/rate
// Dynamic brightness
// Setters for internal values
// Pass in spectrum/audio
// Sound-reactive update for patterns (how?)


// Pass in neopixel rather than extend?
// Support for multiple neopixels used simultaneously (different dimensions?)




// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPattern : public Adafruit_NeoPixel {
public:

    enum pattern { NO_PATTERN, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
    enum direction { FORWARD, REVERSE };
    enum finisher { NO_FINISHER, REVERSAL, RANDOM_COLORS, RANDOM_CHASE, RANDOM_INTERVAL };
 
    pattern ActivePattern;  // which pattern is running
    unsigned long Interval;   // milliseconds between updates
    direction Direction;     // direction to run the pattern
    finisher Finisher;
    uint32_t PrimaryColor;
    uint32_t SecondaryColor;
    //uint32_t TertiaryColor;
    
    uint16_t TotalSteps;  // total number of steps in the pattern
    unsigned long lastUpdate; // last update of position
    uint16_t Index;  // current step within the pattern

    Adafruit_NeoPixel Strip;
    
    void (*OnComplete)();
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPattern(Adafruit_NeoPixel strip, void (*callback)() = NULL) {
        Strip = strip;
        OnComplete = callback;
    }

    void runFinisher() {
      switch (Finisher) {
          case REVERSAL:
              Reverse_finisher();
              break;
          case RANDOM_COLORS:
              RandomColors_finisher();
              break;
          case RANDOM_CHASE:
              RandomChase_finisher();
              break;
          case RANDOM_INTERVAL:
              RandomInterval_finisher();
              break;
          default:
              break;
      }
    }
    
    void Reverse_finisher() {
        Reverse();
    }
    
    void RandomColors_finisher() {
        PrimaryColor = Wheel(random(255));
        SecondaryColor = Wheel(random(255));
        //TertiaryColor = Wheel(random(255));
    }
    
    void RandomChase_finisher() {
        PrimaryColor = SecondaryColor;
        SecondaryColor = Wheel(random(255));
        //SecondaryColor = TertiaryColor;
        //TertiaryColor = Wheel(random(255));
    }
    
    void RandomInterval_finisher() {
        Interval = random(0,10);
    }

    static void blah() {
      
    }

    //Pattern.StartPreset(NeoPattern::SCANNER, Strip.Color(255,0,0), 55, NeoPattern::RANDOM_COLORS);

    void StartPreset(pattern pattern, uint32_t primaryColor, unsigned long interval, finisher finisher, uint32_t secondaryColor, direction startDirection = FORWARD) {
    //void StartPreset(pattern pattern, unsigned long interval, direction startDirection, finisher finisher, uint32_t primaryColor, uint32_t secondaryColor = NULL) {
        // uint32_t tertiaryColor
        ActivePattern = pattern;
        Interval = interval;
        Direction = startDirection;
        Finisher = finisher;
        PrimaryColor = primaryColor;
        SecondaryColor = secondaryColor ?: PrimaryColor;
        //if (SecondaryColor == NULL) {
        //  SecondaryColor = PrimaryColor;
        //}
        //TertiaryColor = tertiaryColor;

        Index = 0;
        StartPattern();
    }

    // Soon to be deprecated by startPreset
    void StartPattern() {
        switch (ActivePattern) {
            case RAINBOW_CYCLE:
                //RainbowCycle(Interval, Direction);
                RainbowCycle_init();
                break;
            case THEATER_CHASE:
                //TheaterChase(Color1, Color2, Interval, Direction);
                TheaterChase_init();
                break;
            case COLOR_WIPE:
                //ColorWipe(Color1, Interval, Direction);
                ColorWipe_init();
                break;
            case SCANNER:
                //Scanner(Color1, Interval);
                Scanner_init();
                break;
            case FADE:
                //Fade(Color1, Color2, TotalSteps, Interval, Direction);
                Fade_init();
                break;
            default:
                break;
        }
    }

    String GetPatternName() {
      switch (ActivePattern) {
          case RAINBOW_CYCLE:
              return "Rainbow Cycle";
              break;
          case THEATER_CHASE:
              return "Theatre Chase";
              break;
          case COLOR_WIPE:
              return "Colour Wipe";
              break;
          case SCANNER:
              return "Scanner";
              break;
          case FADE:
              return "Fade";
              break;
          default:
              return "wat";
              break;
      }
    }
    
    // Update the pattern
    void Update() {
        if ((millis() - lastUpdate) > Interval) {
            lastUpdate = millis();
            switch (ActivePattern) {
                case RAINBOW_CYCLE:
                    RainbowCycle_update();
                    break;
                case THEATER_CHASE:
                    TheaterChase_update();
                    break;
                case COLOR_WIPE:
                    ColorWipe_update();
                    break;
                case SCANNER:
                    Scanner_update();
                    break;
                case FADE:
                    Fade_update();
                    break;
                default:
                    break;
            }
        }
    }
  
    // Increment the Index and reset at the end
    void Increment() {
        if (Direction == FORWARD) {
           Index++;
           if (Index >= TotalSteps) {
                Index = 0;
                runFinisher();
                if (OnComplete != NULL) {
                    OnComplete(); // call the comlpetion callback
                }
            }
        } else { // Direction == REVERSE
            --Index;
            if (Index <= 0) {
                Index = TotalSteps-1;
                runFinisher();
                if (OnComplete != NULL) {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse() {
        if (Direction == FORWARD) {
            Direction = REVERSE;
            Index = TotalSteps-1;
        } else {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    //void RainbowCycle(uint8_t interval, direction dir = FORWARD) {
    void RainbowCycle_init() {
        TotalSteps = 255;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycle_update() {
        for (int i=0; i< Strip.numPixels(); i++) {
            Strip.setPixelColor(i, Wheel(((i * 256 / Strip.numPixels()) + Index) & 255));
        }
        Strip.show();
        Increment();
    }
 
    // Initialize for a Theater Chase
    //void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD) {
    void TheaterChase_init() {
        TotalSteps = Strip.numPixels();
   }
    
    // Update the Theater Chase Pattern
    void TheaterChase_update() {
        for (int i=0; i< Strip.numPixels(); i++) {
            if ((i + Index) % 3 == 0) {
                Strip.setPixelColor(i, PrimaryColor);
            } else {
                Strip.setPixelColor(i, SecondaryColor);
            }
        }
        Strip.show();
        Increment();
    }
 
    // Initialize for a ColorWipe
    //void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD) {
    void ColorWipe_init() {
        TotalSteps = Strip.numPixels();
    }
    
    // Update the Color Wipe Pattern
    void ColorWipe_update() {
        Strip.setPixelColor(Index, PrimaryColor);
        Strip.show();
        Increment();
    }
    
    // Initialize for a SCANNNER
    //void Scanner(uint32_t color1, uint8_t interval) {
    void Scanner_init() {
        TotalSteps = (Strip.numPixels() - 1) * 2;
    }
 
    // Update the Scanner Pattern
    void Scanner_update() { 
        for (int i=0; i< Strip.numPixels(); i++) {
            if (i == Index) { // Scan Pixel to the right
                 Strip.setPixelColor(i, PrimaryColor);
            } else if (i == TotalSteps - Index) { // Scan Pixel to the left
                 Strip.setPixelColor(i, PrimaryColor);
            } else { // Fading tail
                 Strip.setPixelColor(i, DimColor(Strip.getPixelColor(i)));
            }
        }
        Strip.show();
        Increment();
    }
    
    // Initialize for a Fade
    //void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD) {
    void Fade_init() {
        // Was originally configurable... worth it?
        TotalSteps = Strip.numPixels();
    }
    
    // Update the Fade Pattern
    void Fade_update() {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(PrimaryColor) * (TotalSteps - Index)) + (Red(SecondaryColor) * Index)) / TotalSteps;
        uint8_t green = ((Green(PrimaryColor) * (TotalSteps - Index)) + (Green(SecondaryColor) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(PrimaryColor) * (TotalSteps - Index)) + (Blue(SecondaryColor) * Index)) / TotalSteps;
        
        ColorSet(Strip.Color(red, green, blue));
        Strip.show();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color) {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Strip.Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color) {
        for (int i=0; i< Strip.numPixels(); i++) {
            Strip.setPixelColor(i, color);
        }
        Strip.show();
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color) {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color) {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color) {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos) {
        WheelPos = 255 - WheelPos;
        if (WheelPos < 85) {
            return Strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
        } else if(WheelPos < 170) {
            WheelPos -= 85;
            return Strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
        } else {
            WheelPos -= 170;
            return Strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

/*
// Ring1 Completion Callback
void Ring1Complete()
{
    if (digitalRead(9) == LOW)  // Button #2 pressed
    {
        // Alternate color-wipe patterns with Ring2
        Ring2.Interval = 40;
        Ring1.Color1 = Ring1.Wheel(random(255));
        Ring1.Interval = 20000;
    }
    else  // Retrn to normal
    {
      Ring1.Reverse();
    }
}

// Ring 2 Completion Callback
void Ring2Complete()
{
    if (digitalRead(9) == LOW)  // Button #2 pressed
    {
        // Alternate color-wipe patterns with Ring1
        Ring1.Interval = 20;
        Ring2.Color1 = Ring2.Wheel(random(255));
        Ring2.Interval = 20000;
    }
    else  // Retrn to normal
    {
        Ring2.RainbowCycle(random(0,10));
    }
}

// Stick Completion Callback
void StickComplete()
{
    // Random color change for next scan
    Stick.Color1 = Stick.Wheel(random(255));
}
*/

/*
void Ring1Complete();
void Ring2Complete();
void StickComplete();
 
// Define some NeoPatterns for the two rings and the stick
//  as well as some completion routines
NeoPatterns Ring1(24, 5, NEO_GRB + NEO_KHZ800, &Ring1Complete);
NeoPatterns Ring2(16, 6, NEO_GRB + NEO_KHZ800, &Ring2Complete);
NeoPatterns Stick(16, 7, NEO_GRB + NEO_KHZ800, &StickComplete);
*/


// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel Strip = Adafruit_NeoPixel(60, 6, NEO_GRB + NEO_KHZ800);


void StripComplete() {};
//NeoPatterns Strip(60, 6, NEO_GRB + NEO_KHZ800, &StripComplete);
NeoPattern Pattern = NeoPattern(Strip, &StripComplete);

/* *************** COMPLETION ROUTINES ************************************ */


/*
void loop() {
    Strip.Update();
    
    // Switch patterns on a button press:
    if (digitalRead(8) == LOW) { // Button #1 pressed
        // Set strip to all red
        Strip.ColorSet(Strip.Color(255, 0, 0));
    } else if (digitalRead(9) == LOW) { // Button #2 pressed
        // And update tbe strip
        Strip.Update();
    } else { // Back to normal operation
        // And update the strip
        Strip.Update();
    }

    /*
    // Update the rings.
    Ring1.Update();
    Ring2.Update();    
    
    // Switch patterns on a button press:
    if (digitalRead(8) == LOW) { // Button #1 pressed
        // Switch Ring1 to FADE pattern
        Ring1.ActivePattern = FADE;
        Ring1.Interval = 20;
        // Speed up the rainbow on Ring2
        Ring2.Interval = 0;
        // Set stick to all red
        Stick.ColorSet(Stick.Color(255, 0, 0));
    } else if (digitalRead(9) == LOW) { // Button #2 pressed
        // Switch to alternating color wipes on Rings1 and 2
        Ring1.ActivePattern = COLOR_WIPE;
        Ring2.ActivePattern = COLOR_WIPE;
        Ring2.TotalSteps = Ring2.numPixels();
        // And update tbe stick
        Stick.Update();
    } else { // Back to normal operation
        // Restore all pattern parameters to normal values
        Ring1.ActivePattern = THEATER_CHASE;
        Ring1.Interval = 100;
        Ring2.ActivePattern = RAINBOW_CYCLE;
        Ring2.TotalSteps = 255;
        Ring2.Interval = min(10, Ring2.Interval);
        // And update tbe stick
        Stick.Update();
    }
}
*/

// Initialize everything and prepare to start
/*
void setup() {
    Serial.begin(115200);
    
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    
    // Initialize all the pixelStrips
    Strip.begin();
    
    // Kick off a pattern
    Strip.Scanner(Strip.Color(255,0,0), 55);

    /*
    Ring1.TheaterChase(Ring1.Color(255,255,0), Ring1.Color(0,0,50), 100);
    Ring2.RainbowCycle(3);
    Ring2.Color1 = Ring1.Color1;
    Stick.Scanner(Ring1.Color(255,0,0), 55);
}
*/

//const int NumPatterns = 2;

struct Preset {
    String name;
    NeoPattern::pattern Pattern;
    unsigned long Interval;
    NeoPattern::direction Direction;
    NeoPattern::finisher Finisher;
    uint32_t PrimaryColor;
    uint32_t SecondaryColor; // TertiaryColor
};

// One of each (first three are original from demo)
Preset presetList[] = {
    { "TheaterChase", NeoPattern::THEATER_CHASE, 100, NeoPattern::FORWARD, NeoPattern::REVERSAL, Strip.Color(255,255,0), Strip.Color(0,0,50) },
    { "Rainbow Cycle", NeoPattern::RAINBOW_CYCLE, 3, NeoPattern::FORWARD, NeoPattern::RANDOM_INTERVAL, Strip.Color(255,255,0), Strip.Color(0,0,50) },
    { "Scanner", NeoPattern::SCANNER, 55, NeoPattern::FORWARD, NeoPattern::RANDOM_COLORS, Strip.Color(255,0,0), Strip.Color(255,0,0) },
    { "Color Wipe", NeoPattern::COLOR_WIPE, 50, NeoPattern::FORWARD, NeoPattern::RANDOM_CHASE, Strip.Color(255,255,0), Strip.Color(0,0,50) },
    { "Fade", NeoPattern::FADE, 50, NeoPattern::FORWARD, NeoPattern::RANDOM_CHASE, Strip.Color(255,0,0), Strip.Color(255,0,0) },
};

const int NumPresets = sizeof(presetList);

// One of each

/*
struct button {
    uint8_t capPad[2];
    uint8_t pixel[3];
    uint32_t color;
    uint16_t freq;
} simonButton[] = {
  { {3,2},    {0,1,2},  0x00FF00,   415 },  // GREEN
  { {0,1},    {2,3,4},  0xFFFF00,   252 },  // YELLOW
  { {12, 6},  {5,6,7},  0x0000FF,   209 },  // BLUE
  { {9, 10},  {7,8,9},  0xFF0000,   310 },  // RED 
};
*/







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

// Init runtime vars
int logCounter = 0;

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

/* *************** MODES ************************************ */

/*
#define SMART_MODE 0
#define MANUAL_MODE 1
#define AUDIO_MODE 2
#define MODES 2
int MODE_ORDER[MODES] = {MANUAL_MODE, AUDIO_MODE};

int MODE_INDEX = 0;
int MODE = MODE_ORDER[MODE_INDEX];

void cycleMode() {
  MODE_INDEX = (MODE_INDEX + 1) % MODES;
  MODE = MODE_ORDER[MODE_INDEX];
  drawMode();
}

void toggleSoundActive() {
  
}
 */


/*
//const int NumPatterns = 5;
//const int patterns[NumPatterns] = { Strip.RAINBOW_CYCLE, Strip.THEATER_CHASE, Strip.COLOR_WIPE, Strip.SCANNER, Strip.FADE };
const int patterns[NumPatterns] = {
    NeoPattern::RAINBOW_CYCLE,
    NeoPattern::THEATER_CHASE,
    NeoPattern::COLOR_WIPE,
    NeoPattern::SCANNER,
    NeoPattern::FADE
};
*/
int presetIndex = 0;
int patternIndex = 0;

void startPresetByIndex(int presetIndex) {
    Preset preset = presetList[presetIndex];
    Pattern.StartPreset(preset.Pattern, preset.PrimaryColor, preset.Interval, preset.Finisher, preset.SecondaryColor, preset.Direction);
}

void cyclePattern() {
    presetIndex = (presetIndex + 1) % NumPresets;
    startPresetByIndex(presetIndex);

/*
struct Preset {
    String name;
    NeoPattern::pattern Pattern;
    unsigned long Interval;
    NeoPattern::direction Direction;
    NeoPattern::finisher Finisher;
    uint32_t PrimaryColor;
    uint32_t SecondaryColor; // TertiaryColor
};

    void    StartPreset(pattern pattern, uint32_t primaryColor, unsigned long interval, finisher finisher, uint32_t secondaryColor, direction startDirection = FORWARD) {

    */
}

/* *************** DRAWING ************************************ */

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  for (j=0; j<256; j++) {
    for (i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for (j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for (i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void setupStrip() {
  /*
  strip.begin();
  strip.setBrightness(15); //adjust brightness here
  strip.show(); // Initialize all pixels to 'off'
   */
    
  // Initialize all the pixelStrips
  Strip.begin();
  strip.setBrightness(30); //adjust brightness here

  //Pattern.StartPreset(NeoPattern::SCANNER, Strip.Color(255,0,0), 55, NeoPattern::RANDOM_COLORS, Strip.Color(0,0,0));
  //Pattern.Scanner(Strip.Color(255,0,0), 55);
  //patternIndex = 3;
  presetIndex = 0;
  startPresetByIndex(0);
}

void drawStrip() {
  Pattern.Update();
  
  /*
  // Some example procedures showing how to display to the pixels:
  //colorWipe(strip.Color(255, 0, 0), 50); // Red
  //colorWipe(strip.Color(0, 255, 0), 50); // Green
  //colorWipe(strip.Color(0, 0, 255), 50); // Blue
  //rainbow(1);
  rainbowCycle(1);
  */
}

/* *************** LCD INTERFACE ************************************ */

void quickButtonPress() {
  //cycleMode();
  cyclePattern();
}

void longButtonPress() {
  //toggleSoundActive();
}

void handleButtons() {
  uint8_t buttons = lcd.readButtons();
  if (buttons) {
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
    drawLCD();
  }
  LAST_BUTTONS = buttons;
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
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(Pattern.GetPatternName());
}

void drawLCD() {
  drawMode();
}

void handleLCD() {
  handleButtons();
}

/* *************** LOGGING ************************************ */

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

void setupLogging() {
  Serial.begin(9600);
}

/* *************** RUNTIME ************************************ */

void loop() {
  handleLCD();
  
  updateSpectrum();

  drawStrip();
}

/*
void loop() {
    Strip.Update();
    
    // Switch patterns on a button press:
    if (digitalRead(8) == LOW) { // Button #1 pressed
        // Set strip to all red
        Strip.ColorSet(Strip.Color(255, 0, 0));
    } else if (digitalRead(9) == LOW) { // Button #2 pressed
        // And update tbe strip
        Strip.Update();
    } else { // Back to normal operation
        // And update the strip
        Strip.Update();
    }

    /*
    // Update the rings.
    Ring1.Update();
    Ring2.Update();    
    
    // Switch patterns on a button press:
    if (digitalRead(8) == LOW) { // Button #1 pressed
        // Switch Ring1 to FADE pattern
        Ring1.ActivePattern = FADE;
        Ring1.Interval = 20;
        // Speed up the rainbow on Ring2
        Ring2.Interval = 0;
        // Set stick to all red
        Stick.ColorSet(Stick.Color(255, 0, 0));
    } else if (digitalRead(9) == LOW) { // Button #2 pressed
        // Switch to alternating color wipes on Rings1 and 2
        Ring1.ActivePattern = COLOR_WIPE;
        Ring2.ActivePattern = COLOR_WIPE;
        Ring2.TotalSteps = Ring2.numPixels();
        // And update tbe stick
        Stick.Update();
    } else { // Back to normal operation
        // Restore all pattern parameters to normal values
        Ring1.ActivePattern = THEATER_CHASE;
        Ring1.Interval = 100;
        Ring2.ActivePattern = RAINBOW_CYCLE;
        Ring2.TotalSteps = 255;
        Ring2.Interval = min(10, Ring2.Interval);
        // And update tbe stick
        Stick.Update();
    }
}
*/

// Initialize everything and prepare to start
/*
void setup() {
    Serial.begin(115200);
    
    pinMode(8, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    
    // Initialize all the pixelStrips
    Strip.begin();
    
    // Kick off a pattern
    Strip.Scanner(Strip.Color(255,0,0), 55);

    /*
    Ring1.TheaterChase(Ring1.Color(255,255,0), Ring1.Color(0,0,50), 100);
    Ring2.RainbowCycle(3);
    Ring2.Color1 = Ring1.Color1;
    Stick.Scanner(Ring1.Color(255,0,0), 55);
}
*/

void setup() {
  setupLogging();
  setupLCD();
  setupSpectrum();
  setupStrip();

  updateSpectrum();

  drawLCD();
  drawStrip();
}
