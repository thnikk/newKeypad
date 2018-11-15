/* Future Tone Controller
SUMMARY
This controller was made for Hatsune Miku Project Diva
Future Tone for PS4. It also works with Project Diva X,
but despite the DualShock 4's compatibility with the
PSTV, it isn't fully compatible with previous games
since there are no analog sticks.

This controller utilizes both a Brook PS4 Fight
board and a Teensy LC. The inputs are taken into
the Teensy and pushed out to the fight board to
allow for remapping and input processing to control
the LEDs in the controller.


PINOUT
This isn't a 1:1 representation since the layout
is a little bit different on the actuall controller
(up and down keys are directly adjacent, there's
no space in the middle like there is on here) but
that's just because everything needed to be on a
seperate line.

        Labels                  Pins
______________________  ______________________
|  |L1|SH|PS|OP|R1|  |  |  | 9|  |  |  |11|  |
|L2|UP|L3|MD|R3|TR|R2|  |10| 7|  |26|  | 3|12|
|LF|  |RG|  |SQ|  |CR|  | 5|  | 4|  | 1|  | 0|
|  |DN| ______ |CS|  |  |  | 6| ______ | 2|  |
|      | TP  |       |  |      |     |       |
----------------------  ----------------------

*LED pin is 17. Any unlabeled pins connect directly
to the Brook board and do not connect to the Teensy.
Outputs and all other pins are defined below.


LED ADDRESSES
This is the order of the LEDs (how they are wired.)
This is matched with the inputs using the inToColors
array.
______________________
|  |11|  |10|  | 9|  |
|12| 0|  |  |  | 7| 8|
| 1|  | 3|  | 4|  | 6|
|  | 2| ______ | 5|  |
|      |     |       |
----------------------

Written by: thnikk
Website: thnikk.moe
twitter: @thnikk
*/

// Libraries
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Bounce2.h>

#define version 1
#define DEBUG

// LEDs
#define ledPin     17
#define numpixels  13
#define numkeys    8 // Number of keys
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numpixels, ledPin, NEO_GRB + NEO_KHZ800);
byte rgb[numkeys][3]; // RGB color array

// Inputs
const byte input[numpixels] = { 0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 255, 11, 12 };
// Input order for LEDs
const byte inToColors[numpixels] = { 7, 5, 6, 4, 1, 2, 0, 3,  12, 11, 255, 9, 10 };
const byte modePin = 26;

// Output
// These defines are for readability and keep the program size low
#define circle    15
#define square    16
#define cross     18
#define triangle  19
#define right     20
#define left      21
#define down      22
#define up        23
byte output[2][numkeys] ={
  { circle, square, cross, triangle, right, left, down, up },
  { circle, cross, right, down, square, triangle, left, up }
};

// Color definitions
#define red     0
#define orange  28
#define yellow  40
#define green   85
#define blue    170
#define purple  200
#define pink    230

// Color array for modes
// This can be replaced by using inputs to assign colors rahter than doing it manually.
byte colors[2][numpixels] ={
  { green, pink, blue, red, pink, blue, red, green, yellow, yellow, pink, yellow, yellow},
  { green, green, pink, pink, blue, red, red, blue, yellow, yellow, purple, yellow, yellow }
};


byte colorBrightness[numpixels]; // Per-key brightness (processed seperately from b)
byte mode = 0; // Default mode value
byte b = 50; // Brightness

unsigned long previousMillis;
unsigned long debugMillis;
bool test = 0;

// side button
unsigned long modeMillis;
byte hold = 0;
unsigned long s = 500;
unsigned long brightMillis;
byte blink = 0;
Bounce sb = Bounce();

// sweep effect
/*
byte sweepMap[2][7] = {
  { 255, 0, 255, 255, 255, 7, 255 },
  {   1, 2,   3, 255,   4, 5,   6 }
};

bool LR[2] = { 0, 0 };
byte sweepRow[7] = { 0, 0, 0, 0, 0, 0, 0 };
unsigned long sweepMillis[2];

void sweep(){
  if (!digitalRead(12) || !digitalRead(11)) {
    if ((millis() - sweepMillis[0]) < 127) sweepRow[0] = sweepRow[0] + 1;
    if ((millis() - sweepMillis[0]) > 127 && millis() - sweepMillis[0]) < 127*3) {
      sweepRow[0] = sweepRow[0] + 1;

  }
  if (!digitalRead(9) || !digitalRead(10)) LR[1] = 1;  // Right
  if (digitalRead(12) && digitalRead(11)) sweepMillis[0] = millis();
  if (digitalRead(9) && digitalRead(10)) sweepMillis[1] = millis();


}*/

void setup(){
  #if defined (DEBUG)
    Serial.begin(9600);
  #endif
  // Version checker
  if(EEPROM.read(0) != version){ // If version doesn't match, set values to defaults
    EEPROM.write(0, version);
    EEPROM.write(1, mode);
    EEPROM.write(2, b);
  }
  // Load values
  mode = EEPROM.read(1);
  b = EEPROM.read(2);

  // Set pins
  for (byte x=0; x<numpixels; x++) pinMode(input[x], INPUT_PULLUP);
  for (byte x=0; x<numkeys; x++) pinMode(output[0][x], OUTPUT);
  pinMode(modePin, INPUT_PULLUP); // Side button
  sb.attach(modePin); // Load side button into debounce library
  sb.interval(8); // set debounce time

  pixels.begin(); // Start neopixel library
}

void loop(){

  // Convert inputs to outputs
  for (byte x=0; x<numkeys; x++) {
    if (digitalRead(input[x]) == 0 && hold != 2) {
      // pressed[x] = 1;
      digitalWrite(output[mode][x], LOW);
    }
    if (!digitalRead(input[x]) == 0 && hold != 2) {
      // pressed[x] = 0;
      digitalWrite(output[mode][x], HIGH);
    }
  }

  modeButton();

  // LEDs
  if ((millis() - previousMillis) > 1) {
    colorPress();
    // colorSweep(); // writes over original colors
    pixels.show();
    previousMillis = millis();
  }

  #if defined (DEBUG)
  if ((millis() - debugMillis) > 1000) {/*
    for (byte x=0; x < numpixels; x++){
      Serial.print(digitalRead(inToColors[x]));
      if (x!=numpixels-1)Serial.print(", ");
    }
    Serial.println();

    for (byte x=0; x<numpixels; x++){
      Serial.print(colors[mode][x]);
      if (x!=numpixels-1)Serial.print(", ");
    }
    Serial.println();*/

    // side button states
    Serial.print(digitalRead(modePin));
    Serial.print(", ");
    Serial.print(sb.read());
    Serial.print(", ");
    Serial.println(mode);
    debugMillis = millis();
  }
  #endif

}

void modeButton() {

  sb.update(); // update state of side button

  if (!sb.read()) {
  // Press action: Sets hold value depending on how long the side button is held
    if ((millis() - modeMillis) < s)  hold = 1;
    if ((millis() - modeMillis) > s)  hold = 2;
  }
  // Release action
  if (sb.read()) {
    // Press and release escape
    if (hold == 1) {
      mode++;
      if (mode > 1) mode = 0;
      EEPROM.write(1, mode);
    }
    // Save brightness
    if (hold == 2) {
      EEPROM.write(2, b);
    }
    hold = 0;
    blink = 0;
    modeMillis = millis();
  }

  // brightness changer
  if (hold == 2) {

    if (blink == 0) { // blink twice, reset when key is released. Delay locks it into finishing.
      for (byte y=0; y<2; y++) {
        for(byte x=0; x<numpixels; x++) pixels.setPixelColor(x, pixels.Color(0, 0, 0));
        pixels.show();
        delay(20);
        for(byte x=0; x<numpixels; x++) pixels.setPixelColor(x, pixels.Color(255*b/255, 255*b/255, 255*b/255));
        pixels.show();
        delay(50);
      }
      blink = 1;
    }
    if ((millis() - brightMillis) > 10) {
      for (byte y=0; y<numkeys; y++){
        if (output[mode][y] == up) if (!digitalRead(input[y])) if (b < 255) b++;
        if (output[mode][y] == down) if (!digitalRead(input[y])) if (b > 50) b--;
      }
      brightMillis = millis();
    }
  }
}

// Checks to see if a key is pressed and changes colorBrightness values accordingly
void colorPress(){
  for (byte x=0; x < numpixels; x++){
    wheel(colors[mode][x], x);
    setLED(x);
  }
  for (byte z=0; z<13; z++) { // Fades colors
    if (digitalRead(inToColors[z]) && colorBrightness[z] < 255) colorBrightness[z] = colorBrightness[z] + 3;
    if (!digitalRead(inToColors[z])) colorBrightness[z] = 0;
  }
}

// Converts byte to RGB value
// This is a lot less granular than it could be, but it's also a lot more efficient.
void wheel(byte shortColor, byte key) {
  if (shortColor >= 0 && shortColor < 85) { rgb[key][0] = (shortColor * -3) +255; rgb[key][1] = shortColor * 3; rgb[key][2] = 0; }
  else if (shortColor >= 85 && shortColor < 170) { rgb[key][0] = 0; rgb[key][1] = ((shortColor - 85) * -3) +255; rgb[key][2] = (shortColor - 85) * 3; }
  else { rgb[key][0] = (shortColor - 170) * 3; rgb[key][1] = 0; rgb[key][2] = ((shortColor - 170) * -3) +255; }
}

// Sets LED with universal and per-key brightness (for effects)
void setLED(byte led) {
  byte rgbBuff[3]; // create buffer for tgb values of the given LED
  for(byte x=0; x<3; x++) { // r, g, and b
    rgbBuff[x] = rgb[led][x]; // load value into buffer
    rgbBuff[x] = b*rgbBuff[x]/255; // Brightness multiplier
    if (led != 10) rgbBuff[x] = colorBrightness[led]*rgbBuff[x]/255; // colorPress multiplier
  }
  pixels.setPixelColor(led, pixels.Color(rgbBuff[0], rgbBuff[1], rgbBuff[2])); // write to pixels
}
