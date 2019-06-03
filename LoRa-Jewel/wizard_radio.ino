// https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/using-the-rfm-9x-radio

#include <SPI.h>
#include <RH_RF95.h>
#include <Adafruit_NeoPixel.h>

/* *************** CONFIGURATION ************************************ */

#define RFM95_CS 8
#define FRM95_RST 4
#define RFM95_INT 3
// must match RX's freq, 915 or 868MHz
#define RF95_FREQ 915.0

// Used for wizard id by receiver, might be simpler to assign a number
#define BLUE_WIZ "blue  "
#define GREEN_WIZ "green "
#define RED_WIZ "red   "
#define PURP_WIZ "purple"
// Transmitting wizard
#define CURR_WIZ = BLUE_WIZ

#define TX_COLOR = roygtbv[5]

// Lights
#define JEWEL_PIN 0
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(7, JEWEL_PIN, NEO_GRBW + NEO_KHZ800);

// determine method to call from RaveNodes
#define TX_MODE 0;
#define RX_MODE 1;
#define DRAW_MODE 2;

// should probably omit dist_mid, not enough pixels
#define DIST_FAR 3;
#define DIST_MID 2;
#define DIST_NEAR 1;

/* *********** Globals ************** */
// add more vars for each "node/wizard/raver"
int BLUE_DIST = 0;  // set broadcasting color to 0
int RED_DIST = 99; // init other colors 
int GREEN_DIST = 99;


class RaveNode {
  long OnTime; //millisec to stay "on"
  long OffTime; //millisec to stay "off"

  // Current state
  unsigned long previousMs;
  int callable;

  public:
  RaveNode(int method, long on, long off) {
    OnTime = on;
    OffTime = off;
    callable = method;

    previousMs = 0;   
  }

  boolean shouldRun() {
    if (currentMs - previousMs >= OnTime) {
      //do thing, task still has time remaining
      previousMs = currentMs;
      return true;
    } else if (currentMs - previousMs >= OffTime) {
      //do thing, task has been off/not run for duration
      previousMs = currentMs;
      return true;
    } else {
      return false;
    }
  }
  
  void Run() {
    unsigned long currentMs = millis();
    if ((callable == TX_MODE) && (shouldRun())) {
      transmit();
    } else if ((callable == RX_MODE) && (shouldRun())) {
      receive();
    } else if ((callable == DRAW_MODE) && (shouldRun())) {
      colorThings();
    }
  }
}


// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
// Setup workers, durations will definitley need to be tuned
RaveNode sender(TX_MODE, 1000, 10000); // method, on time, off time
RaveNode receiver(RX_MODE, 60000, 10000);
RaveNode lights(DRAW_MODE, 60000, 5000);

void setupRadio() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    while (1);
  } 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(10, false);
}

void setupLights() {
  pixels.begin();
  pixels.setBrightness(15);
  pixels.show();
}

void setup() {
  setupRadio();
  setupLights();
}

void loop() {
  sender.Run();
  receiver.Run();
  lights.Run();
}


void transmit() {

  char radiopacket[5] = CURR_WIZ;
  itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); 
  Serial.println(radiopacket);
  radiopacket[19] = 0;

  Serial.println("Sending...");
  delay(10);
  rf95.send((uint8_t *)radiopacket, 20);

  Serial.println("Waiting for packet to complete..."); 
  delay(10);
  rf95.waitPacketSent();

}

void receive() {
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
 
    if (rf95.recv(buf, &len)) {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      updateColors((char*)buf, rf95.lastRssi());      
    } else {
      Serial.println("Receive failed");
    }
  }
}

// sig strength range is -15 to -100 (strongest to weakest)
int calcDistance(int rx_pow) {
  if (rx_pow >= -40) {
    return DIST_NEAR;
  } else if ((rx_pow > -40) && (rx_pow > -75)) {
    return DIST_MID;
  } else {
    return DIST_FAR;
  }
}

// update globals based on received signal strength
void updateColors(char *rx_color[], int rx_pow) {
  String id_color(rx_color);
  int dist = calcDistance(rx_pow);
  switch (id_color) {
    case "blue":
      //don't update color of transmitter
      break;
    case "red":
      RED_DIST = dist;
      break;
    case "green": 
      GREEN_DIST = dist;
      break;
    case "purple":
      PURPLE_DIST = dist;
      break;
    default:
      break;
  }
}

void drawColors(uint8_t wait, uint32_t cycles) {
  //set Jewel center to transmitter color
  pixels.setPixelColor(pixels.numPixels(), TX_COLOR);
  for (int c = 0; c < cycles; c++) {
    for (int color = 0; color < 1024; color++) {
      for (int led = 0; led < pixels.numPixels() - 1; led++) {
        pixels.setPixelColor(led, RgbwBigWheel(color+led));
      }
      
      pixels.show();
      delay(wait);
    }
  }
}

uint32_t calcColorShare() {
  int num_red = 0;
  int num_green = 0;
  if (RED_DIST == DIST_FAR) {
    num_red++;
  } else if (RED_DIST == DIST_MID) {
    num_red += 2;
  } else if (RED_DIST == DIST_NEAR) {
    //interpolate colors or some such
  }

  if (GREEN_DIST == DIST_FAR) {
    num_green++;
  } else if (GREEN_DIST == DIST_MID) {
    num_green += 2;
  } else if (GREEN_DIST == DIST_NEAR) {
    //interpolate colors or some such
  }

  int color_array[pixels.numPixels()];
  int index1;
  if (num_red != 0) {
     for (int i = 0; i < num_red; i++) {
      color_array[i] = roygtbv[0];
      index1 = i;
     }
  }
  if (num_green != 0) {
    for (int i = 0; i <= num_green; i++) {
      int j = index1 + 1 + i;
      color_array(j) = roygtbv[3];
      index1 = j;
    }
  }
  ///AAND SO ON
}

/* ----------Spilly Lights---------------*/
// https://github.com/echobass/Arduino-Projects/blob/master/Hat-Jewel/Hat-Jewel.ino

// todo set these as def vars
const uint32_t roygtbv[] = {
  pixels.Color(255, 0, 0, 0), // Red
  pixels.Color(255, 102, 34, 0), // Orange
  pixels.Color(255, 218, 33, 0), // Yellow
  pixels.Color(51, 221, 0, 0), // Green
  pixels.Color(17, 180, 180, 0), // Teal
  pixels.Color(17, 70, 255, 0), // Blue
  pixels.Color(180, 0, 255, 0) // Violet
};

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
