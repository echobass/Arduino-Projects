#include <Adafruit_NeoPixel.h>
 
#define JEWEL_PIN 0
 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(7, JEWEL_PIN, NEO_GRBW + NEO_KHZ800);
 
void setup() {
  pixels.begin();
  pixels.setBrightness(20);
}
 
void loop() {
  rainbowScanner(80, 1, 16, false, 0.02);
  prismRainbowCycle(1, 20); // Fast prism, 20 cycles
  prismRainbow(10, 2);
  fullRainbow(5, 4);
  ringRainbow(8, 2);
  rainbowScanner(40, 4, 32, true, 0);
  rainbowScanner(40, 4, 32, false, 0);
  rainbowScanner(40, 4, 32, true, 0);
  prismRainbowCycle(1, 20); // Fast prism, 20 cycles
  rainbowScanner(60, 4, 64, false, 0);
  rainbowScanner(60, 4, 64, true, 0);
  rainbowScanner(60, 4, 64, false, 0);
  centerRainbow(2, 1);
  rainbowScanner(60, 2, 32, true, 0.05);
  rainbowScanner(60, 2, 32, true, 0.3);
  rainbowScanner(60, 2, 64, true, 0.3);
  rainbowScanner(60, 2, 32, true, 0.3);
  rainbowScanner(60, 2, 32, false, 0.05);
  centerRainbow(2, 1);
  rainbowScanner(40, 1, 32, false, 0.05);
  ringRainbow(8, 1);
  prismRainbow(10, 2);
  fullRainbow(5, 4);
  prismRainbowCycle(1, 20);
  rainbowScanner(40, 1, 8, true, 0.6);
  prismRainbow(5, 4);
  fullRainbow(5, 4);
  prismRainbow(5, 4);
  fullRainbow(5, 4);
  prismRainbowCycle(1, 20);
  rainbowScanner(60, 1, 64, false, 0.2);
  centerRainbow(random(0, 3), random(1, 3));
  rainbowScanner(60, 1, 8, true, 0);
  rainbowScanner(40, 1, 8, false, 0);
  centerRainbow(random(0, 3), random(1, 3));
  rainbowScanner(80, 4, 4, true, 0.8);
  rainbowScanner(80, 4, 1, false, 0.8);
  centerRainbow(random(0, 3), random(1, 3));
  rainbowScanner(40, 6, 16, true, 0.2);
  centerRainbow(random(0, 3), random(1, 3));
  rainbowScanner(60, 4, 2, false, 0.5);
  rainbowScanner(20, 4, 16, true, 0.5);
  rainbowScanner(60, 4, 32, true, 0.5);
  centerRainbow(random(0, 3), random(1, 3));
  rainbowScanner(60, 1, 32, true, 0.2);
  prismRainbowCycle(1, 20);
  ringRainbow(8, 1);
}

void prismRainbowCycle(uint8_t wait, uint32_t cycles) {
  for (int c = 0; c < cycles; c++) {
    for (int color = 0; color < 1024; color++) {
      pixels.setPixelColor(0, 0x30808080);
      
      for (int led = 1; led < pixels.numPixels(); led++) {
        pixels.setPixelColor(led, RgbwBigWheel(((led * 1024 / (pixels.numPixels()-1)) + color) & 1023));
      }
      
      pixels.show();
      delay(wait);
    }
  }
}

void prismRainbow(uint8_t wait, uint32_t cycles) {
  for (int c = 0; c < cycles; c++) {
    for (int color = 0; color < 1024; color++) {
      pixels.setPixelColor(0, 0x30808080);
      
      for (int led = 1; led < pixels.numPixels(); led++) {
        pixels.setPixelColor(led, RgbwBigWheel(color+led));
      }
      
      pixels.show();
      delay(wait);
    }
  }
}

void ringRainbow(uint8_t wait, uint32_t cycles) {
  pixels.setPixelColor(0, 0);
  for (int c = 0; c < cycles; c++) {
    for (int color = 0; color < 1024; color++) {
      for (int led = 1; led < pixels.numPixels(); led++) {
        pixels.setPixelColor(led, RgbwBigWheel(color+led));
      }
      
      pixels.show();
      delay(wait);
    }
  }
}

void fullRainbow(uint8_t wait, uint32_t cycles) {
  for (int c = 0; c < cycles; c++) {
    for (int color = 0; color < 1024; color++) {
      for (int led = 0; led < pixels.numPixels(); led++) {
        pixels.setPixelColor(led, RgbwBigWheel(color+led));
      }
      
      pixels.show();
      delay(wait);
    }
  }
}

void centerRainbow(uint8_t wait, uint32_t cycles) {
  for (int c = 0; c < cycles; c++) {
    for (int color = 0; color < 1024; color++) {
      pixels.setPixelColor(0, RgbwBigWheel(color));
      if (color % 32 == 0) {
        for (int i = 1; i < pixels.numPixels(); i++) {
          pixels.setPixelColor(i, DimColor(pixels.getPixelColor(i)));
        }
      }
      pixels.show();
      delay(wait);
    }
  }
}

// Interpolate between two colors (RGBW)
// Randomly choose next color

uint32_t interpolate(uint32_t primary, uint32_t secondary, float sigma = 0.5) {
  float alpha = sigma * 2.0;
  float beta = (1 - sigma) * 2.0;
  return pixels.Color(
    (int)(Red(primary)*alpha + Red(secondary)*beta) >> 1,
    (int)(Green(primary)*alpha + Green(secondary)*beta) >> 1,
    (int)(Blue(primary)*alpha + Blue(secondary)*beta) >> 1,
    (int)(White(primary)*alpha + White(secondary)*beta) >> 1
  );
}

void rainbowScanner(uint8_t wait, uint32_t cycles, uint16_t increment, boolean forward, float randomness) {
  uint16_t stepsBetweenColors = 3;
  uint32_t currentStep = 0;
  uint32_t currentColor = 0;
  
  uint32_t color = DimColor(RgbwBigWheel(currentColor));
  pixels.setPixelColor(0, color);

  while (currentColor < (1024*cycles)) {
    for (int m=1; m< pixels.numPixels(); m++) {
      if (currentStep++ >= stepsBetweenColors) {
        currentColor = currentColor + increment;
        color = DimColor(RgbwBigWheel(currentColor));
        pixels.setPixelColor(0, color);
        currentStep = 0;
      }
      
      if (random(100) < (randomness * 100)) {
        forward = !forward;
      }

      if (forward) {
        for (int i=1; i< pixels.numPixels(); i++) {
          if (i == m) {
               pixels.setPixelColor(i, color);
          } else { // Fading tail
               pixels.setPixelColor(i, DimColor(pixels.getPixelColor(i)));
          }
        }
      } else {
        for (int i=pixels.numPixels(); i>= 1; i--) {
          if (i == (pixels.numPixels()-m)) {
               pixels.setPixelColor(i, color);
          } else { // Fading tail
               pixels.setPixelColor(i, DimColor(pixels.getPixelColor(i)));
          }
        }
      }
      
      pixels.show();
      delay(wait);
    }
  }
}

/*
void randomRainbowScanner(uint8_t wait, uint16_t increment, boolean forward) {
  uint16_t stepsBetweenColors = 3;
  uint16_t currentStep = 0;
  uint16_t currentColor = 0;

  // Receive a color from palette to start
  // Choose the next color randomly
  // Interpolate between them while drawing
  // Choose the next color when reached
  // Repeat
  // Break after all colors used
  
  uint32_t color = DimColor(RgbwBigWheel(currentColor));
  pixels.setPixelColor(0, color);

  while (currentColor < 1024) {
    for (int m=1; m< pixels.numPixels(); m++) {
      if (currentStep++ >= stepsBetweenColors) {
        currentColor = currentColor + increment;
        color = DimColor(RgbwBigWheel(currentColor));
        pixels.setPixelColor(0, color);
        currentStep = 0;
      }

      if (forward) {
        for (int i=1; i< pixels.numPixels(); i++) {
          if (i == m) {
               pixels.setPixelColor(i, color);
          } else { // Fading tail
               pixels.setPixelColor(i, DimColor(pixels.getPixelColor(i)));
          }
        }
      } else {
        for (int i=pixels.numPixels(); i>= 1; i--) {
          if (i == (pixels.numPixels()-m)) {
               pixels.setPixelColor(i, color);
          } else { // Fading tail
               pixels.setPixelColor(i, DimColor(pixels.getPixelColor(i)));
          }
        }
      }
      
      pixels.show();
      delay(wait);
    }
  }
}
 */

const uint32_t roygtbv[] = {
  pixels.Color(255, 0, 0, 0), // Red
  pixels.Color(255, 102, 34, 0), // Orange
  pixels.Color(255, 218, 33, 0), // Yellow
  pixels.Color(51, 221, 0, 0), // Green
  pixels.Color(17, 180, 180, 0), // Teal
  pixels.Color(17, 70, 255, 0), // Blue
  pixels.Color(180, 0, 255, 0) // Violet
};

void prismScanner(uint8_t wait) {
  
}

uint32_t RgbwBigWheel(int WheelPos) {
  WheelPos = WheelPos % 1024;
  // RGBW
  /*
  if (WheelPos < 256) {
    WheelPos = WheelPos % 256;
    return pixels.Color(WheelPos, 0, 0, 255-WheelPos >> 1);//min(255-WheelPos+32, 255));
  } else if (WheelPos < 512) {
    WheelPos = WheelPos % 256;
    return pixels.Color(255-WheelPos, WheelPos, 0, 0);//32-(WheelPos/32));
  } else if (WheelPos < 768) {
    WheelPos = WheelPos % 256;
    return pixels.Color(0, 255-WheelPos, WheelPos, 0);
  } else if (WheelPos < 1024) {
    WheelPos = WheelPos % 256;
    return pixels.Color(0, 0, 255-WheelPos, WheelPos >> 1);
  }
  */
  // GBRW
  if (WheelPos < 256) {
    WheelPos = WheelPos % 256;
    return pixels.Color(0, WheelPos, 0, (255-WheelPos) >> 2);//min(255-WheelPos+32, 255));
  } else if (WheelPos < 512) {
    WheelPos = WheelPos % 256;
    return pixels.Color(0, 255-WheelPos, WheelPos, 0);//32-(WheelPos/32));
  } else if (WheelPos < 768) {
    WheelPos = WheelPos % 256;
    return pixels.Color(WheelPos, 0, 255-WheelPos, 0);
  } else if (WheelPos < 1024) {
    WheelPos = WheelPos % 256;
    return pixels.Color(255-WheelPos, 0, 0, WheelPos >> 2);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
   
// Calculate 50% dimmed version of a color (used by ScannerUpdate)
uint32_t DimColor(uint32_t color) {
    // Shift R, G and B components one bit to the right
    uint32_t dimColor = pixels.Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1, White(color) >> 1);
    return dimColor;
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

// Returns the Blue component of a 32-bit color
uint8_t White(uint32_t color) {
    return (color >> 24) & 0xFF;
}
