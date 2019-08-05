#include <Adafruit_NeoPixel.h>
#include <Adafruit_CircuitPlayground.h>
#include <EasyButton.h> // https://github.com/evert-arias/EasyButton/

#define KNOB_PIN A6   // The potentiometer
#define LED_PIN 6     // The main LED output

/*
// https://learn.adafruit.com/adafruit-circuit-playground-express/adapting-sketches-to-m0
#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif
 */

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(252, LED_PIN, NEO_GRB + NEO_KHZ800);

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

// Outer triangle = 16 * 3 pixels = 48 pixels
//  0-23, 102-125
//    126-149, 228-251
// Inner hexagon = 13 * 6 pixels = 78 pixels
//  24-101
//    150-227
// Dimmed inner hexagon = 78 / 2 = 39 pixels

// First side = 126 pixels
//  0-125
// Second side = 126 pixels
//  126-251
// = 252 pixels total

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

// TODO
//  Light sensor logging
//  Playground LED animation
//  Playground audio reactivity

// Function to modulate & reverse the cycleIndex progression

EasyButton leftButton(CPLAY_LEFTBUTTON, 300, false, false);
EasyButton rightButton(CPLAY_RIGHTBUTTON, 300, false, false);

int currentPattern = 0;
boolean ledsOn = true;
float speedModifier = 1.0;

int patternTimePeriodMs = 1 * 1000;
int patternCycleIndex = 0;
unsigned long patternLastDraw = 0;

int cyclesBeforeReverse = 13;
boolean reversed = false;
int cycleNumber = 0;
int randomReverseChance = 255 * 7;

/*
void tickPatternCycleIndex() {
  patternCycleIndex = (patternCycleIndex + 1) % 256;
}
 */

void tickPatternCycleIndex() {
  if (cycleNumber == cyclesBeforeReverse) {
    reversed = true;
  } else if (cycleNumber == 0) {
    reversed = false;
  }
  if (random(0, randomReverseChance) == 1) {
    reversed = !reversed;
  }
  if (reversed) {
    patternCycleIndex = (patternCycleIndex - 1) % 256;
    if (patternCycleIndex == 0) {
      cycleNumber = (cycleNumber - 1) % cyclesBeforeReverse;
    }
  } else {
    patternCycleIndex = (patternCycleIndex + 1) % 256;
    if (patternCycleIndex == 255) {
      cycleNumber = (cycleNumber + 1) % cyclesBeforeReverse;
    }
  }
  //patternCycleIndex = (patternCycleIndex + 1) % 256;
  //cycleNumber = (cycleNumber + 1) % 13;
}

void setup() {
  Serial.begin(9600);
  
  CircuitPlayground.begin();

  ledsOn = CircuitPlayground.slideSwitch();

  pinMode(KNOB_PIN, INPUT);
  
  strip.begin();
  strip.setBrightness(100);
  strip.show(); // Initialize all pixels to 'off'

  leftButton.begin();
  leftButton.onPressedFor(300, leftOnPressedCallback);

  //randomSeed(analogRead(KNOB_PIN)+analogRead(1));
  
  /* Right button on current board is stuck and unreliable
  rightButton.begin();
  rightButton.onPressedFor(300, rightOnPressedCallback);
   */
}

void leftOnPressedCallback() {
  currentPattern = currentPattern + 1;
  Serial.print("left ");
  Serial.println(currentPattern);
}

void rightOnPressedCallback() {
  currentPattern = currentPattern - 1;
  Serial.print("right ");
  Serial.println(currentPattern);
}

void loop() {
  // Break if LEDs are off
  if (CircuitPlayground.slideSwitch()) {
    if (!ledsOn) {
      strip.clear();
      strip.show();
    }
    ledsOn = false;
    return;
  }
  ledsOn = true;

  // Mark the time for future needs
  unsigned long now = millis();

  // Read the switches and trigger any callbacks
  leftButton.read();
  rightButton.read();

  // Read the knob for speed modification
  speedModifier = (analogRead(KNOB_PIN) / 512.0); // Center => 1.0 (1024/2)
  if (speedModifier < 1) { // Boost depth of speed mod
    speedModifier = speedModifier / 4.0;
  } else {
    speedModifier = speedModifier * 4.0;
  }
  speedModifier = speedModifier + 0.001; // Minimum speed
  
  /*
  int innerTimePeriodMs = 2.5 * 1000;
  int outerTimePeriodMs = 12.5 * 1000;
  int patternTimePeriodMs = 1 * 1000;
   */

  if (now > (((patternTimePeriodMs/255)/speedModifier) + patternLastDraw)) {
    if (currentPattern == 0) {
      /* Slow Big Single Rainbow */
      /* Nice but slow */
      renderSingleStrip();
    } else if (currentPattern == 1) {
      /* Fast triangle with fast gem */
      /* Good but a bit too fast, center not bright enough */
      renderRainbowGem();
    } else if (currentPattern == 2) {
      /* Prism triangle and gem fade */
      /* Pretty sweet, vibrant */
      renderPrismJewel();
    } else if (currentPattern == 3) {
      /* Rainbow triangle and gem fade */
      /* Really good */
      renderRainbowJewel();
    } else if (currentPattern == 4) {
      /* Crazy rainbows */
      /* Pretty cool, a bit fast */
      renderSingleInvertReversePhasedTriangle(); // Not very good?
    } else if (currentPattern == 5) {
      /* Reversed rainbows */
      /* Good, a bit fast */
      renderSingleInvertReverseTriangle();
    } else if (currentPattern == 6) {
      /* Synchronized rainbows */
      /* Great, a bit too fast */
      renderSingleTriangle();
    } else if (currentPattern == 7) {
      /* Full fade */
      /* Really good, a bit fast */
      renderSingleFade();
    } else if (currentPattern == 8) {
      /* Opposite fade */
      /* Pretty good */
      renderDoubleFade();
      /*
    } else if (currentPattern == ) {
      /*  */
      /*  */
      /* ALSO UPDATE THE IF/ELSE
      if (now > ((patternTimePeriodMs/255) + patternLastDraw)) {
        patternLastDraw = now;
      }
      */
    } else if (currentPattern == -1) {
      currentPattern = 8;
    } else { // Reached the end, reset to 0
      currentPattern = 0;
    }
  
    // Erase every multiple inner LED
    int lightDensity = 2; // 2+
    for (int i = 24; i < 102; i++) {
      if ((i % lightDensity) == 0) {
        strip.setPixelColor(i, 0);
      }
    }
    
    patternLastDraw = now;

    tickPatternCycleIndex();
  
    // Mirror the two sides
    mirror(0, 125);
  
    // Send the strip
    strip.show();
  }
}

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

void mirror(int startPixel, int endPixel) {
  for (int i = startPixel; i <= endPixel; i++) {
    strip.setPixelColor(252-1-i, strip.getPixelColor(i));
  }
}

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

void renderRainbowGem() {
  renderInnerHexagonRainbow();
  renderOuterTriangleFade(); // Should be different speeds...
}

void renderInnerHexagonRainbow() {
  int cycles = 1;
  for (int i = 0; i < 78; i++) {
    int pixel = i + 24;
    int color = Wheel(((i * 256 / (int)(78*cycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(pixel, color);
  }
}

void renderOuterTriangleRainbow() {
  int cycles = 1;
  for (int i = 0; i < 48; i++) {
    int pixel = i;
    if (pixel > 23) pixel = pixel + 78;
    int color = Wheel(((i * 256 / (int)(48*cycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(pixel, color);
  }
}

void renderOuterTriangleFade() {
  int color = Wheel((patternCycleIndex) & 255);
  for (int i = 0; i < 8; i++) {
    // Wat
    strip.setPixelColor(i, color);
    strip.setPixelColor(125-i, color);
    strip.setPixelColor(i+8, color);
    strip.setPixelColor(i+16, color);
    strip.setPixelColor(i+102, color);
    strip.setPixelColor(i+110, color);
  }
}

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

void renderSingleFade() {
  int color = Wheel((patternCycleIndex) & 255);
  for (int i = 0; i < 252; i++) {
    strip.setPixelColor(i, color);
  }
  patternCycleIndex = (patternCycleIndex + 1) % 256;
}

void renderDoubleFade() {
  int innerColor = Wheel((patternCycleIndex) & 255);
  int outerColor = Wheel(((patternCycleIndex) + 127) & 255);
  for (int i = 0; i < 48; i++) {
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    strip.setPixelColor(outerPixel, outerColor);
  }
  for (int i = 0; i < 78; i++) {
    int innerPixel = i + 24;// + 39;
    if (innerPixel > 101) innerPixel = innerPixel - 78;
    strip.setPixelColor(innerPixel, innerColor);
  }
}

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

 void renderSingleStrip() {
  for (int i = 0; i < 126; i++) {
    int cycles = 1;
    int outerColor = Wheel(((i * 256 / (int)(126*cycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(i, outerColor);
  }
 }

void renderSingleTriangle() {
  for (int i = 0; i < 48; i++) {
    int outerCycles = 1;
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    int outerColor = Wheel(((i * 256 / (int)(48*outerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(outerPixel, outerColor);
  }
  
  for (int i = 0; i < 78; i++) {
    int innerCycles = 1;
    int innerPixel = i + 24 + 39;
    if (innerPixel > 101) innerPixel = innerPixel - 78;
    int innerColor = Wheel(((i * 256 / (int)(78*innerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(innerPixel, innerColor);
  }
}

void renderDoubleInvertTriangle() {
  for (int i = 0; i < 48; i++) {
    int outerCycles = 1;
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    int outerColor = Wheel(((i * 256 / (int)(48*outerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(outerPixel, outerColor);
  }
  
  for (int i = 0; i < 78; i++) {
    int innerrCycles = 1;
    int innerPixel = i + 24;
    int innerColor = Wheel(((i * 256 / (int)(78*innerrCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(innerPixel, innerColor);
  }
}

void renderSingleInvertReverseTriangle() {
  for (int i = 0; i < 48; i++) {
    int outerCycles = 1;
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    int outerColor = Wheel(((i * 256 / (int)(48*outerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(outerPixel, outerColor);
  }
  
  for (int i = 0; i < 78; i++) {
    int innerrCycles = 1;
    int innerPixel = 101 - i;
    int innerColor = Wheel(((i * 256 / (int)(78*innerrCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(innerPixel, innerColor);
  }
}

void renderSingleInvertReversePhasedTriangle() {
  for (int i = 0; i < 48; i++) {
    int outerCycles = 3;
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    int outerColor = Wheel(((i * 256 / (int)(48*outerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(outerPixel, outerColor);
  }
  
  for (int i = 0; i < 78; i++) {
    int innerCycles = 1;
    int innerPixel = 101 - i;
    int innerColor = Wheel(((i * 256 / (int)(78*innerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(innerPixel, innerColor);
  }
}

void renderPrismJewel() {
  for (int i = 0; i < 48; i++) {
    int outerCycles = 3;
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    int outerColor = Wheel(((i * 256 / (int)(48/outerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(outerPixel, outerColor);
  }

  int innerColor = Wheel((patternCycleIndex) & 255);
  for (int i = 24; i <= 101; i++) {
    strip.setPixelColor(i, innerColor);
  }
}

void renderRainbowJewel() {
  for (int i = 0; i < 48; i++) {
    int outerCycles = 1;
    int outerPixel = i;
    if (outerPixel > 23) outerPixel = outerPixel + 78;
    int outerColor = Wheel(((i * 256 / (int)(48/outerCycles)) + patternCycleIndex) & 255);
    strip.setPixelColor(outerPixel, outerColor);
  }

  int innerColor = Wheel((patternCycleIndex) & 255);
  for (int i = 24; i <= 101; i++) {
    strip.setPixelColor(i, innerColor);
  }
}

/*
 * ********************************************************** **********************************************************
 * ********************************************************** **********************************************************
 */

 /*
  * https://votecharlie.com/blog/2018/08/improved-color-wheel-function.html
  */
/*
// Input values 0 to 255 to get color values that transition R->G->B. 0 and 255
// are the same color. This is based on Adafruit's Wheel() function, which used
// a linear map that resulted in brightness peaks at 0, 85 and 170. This version
// uses a quadratic map to make values approach 255 faster while leaving full
// red or green or blue untouched. For example, Wheel(42) is halfway between
// red and green. The linear function yielded (126, 129, 0), but this one yields
// (219, 221, 0). This function is based on the equation the circle centered at
// (255,0) with radius 255:  (x-255)^2 + (y-0)^2 = r^2
unsigned long Wheel(byte position) {
  byte R = 0, G = 0, B = 0;
  if (position < 85) {
    R = sqrt32((1530 - 9 * position) * position);
    G = sqrt32(65025 - 9 * position * position);
  } else if (position < 170) {
    position -= 85;
    R = sqrt32(65025 - 9 * position * position);
    B = sqrt32((1530 - 9 * position) * position);
  } else {
    position -= 170;
    G = sqrt32((1530 - 9 * position) * position);
    B = sqrt32(65025 - 9 * position * position);
  }
  return strip.Color(R, G, B);
}

// Adapted from https://www.stm32duino.com/viewtopic.php?t=56#p8160
unsigned int sqrt32(unsigned long n) {
  unsigned int c = 0x8000;
  unsigned int g = 0x8000;
  while(true) {
    if(g*g > n) {
      g ^= c;
    }
    c >>= 1;
    if(c == 0) {
      return g;
    }
    g |= c;
  }
}
 */

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  return strip.ColorHSV(WheelPos*257);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t WheelHSV(byte WheelPos) {
  return strip.ColorHSV(WheelPos*257);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t WheelFlat(byte WheelPos) {
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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t SineWheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(strip.sine8(WheelPos * 3), strip.sine8(255 - WheelPos * 3), 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(strip.sine8(255 - WheelPos * 3), 0, strip.sine8(WheelPos * 3));
  } else {
   WheelPos -= 170;
   return strip.Color(WheelPos, strip.sine8(WheelPos * 3), strip.sine8(255 - WheelPos * 3));
  }
}
