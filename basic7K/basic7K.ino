/*************************************************
  Basic/Basic+ Keypad
    Written for Pro Micro/Leonardo. Features
    1000hz polling rate and button remapper. Can
    be upgraded to LED fairly easily by wiring
    positive ends of the LEDs to pin 5 and 6.

    As of 2/5/17, uses new remapper that allows
    for more modifiers to be used. Also prevents
    false-triggering making the keypad enter
    the remapper by requiring an input of 0
    before use.

  Key Numbering
    ---------
  - | 1 | 2 |
    ---------
  ^ Side button is key 3
  *Pinout is 2, 3, 4 for buttons 1, 2, and 3

  For more info, please visit:
  ----> http://thnikk.moe/
  ----> https://www.etsy.com/shop/thnikk
  ----> twitter.com/thnikk
  ----> https://github.com/thnikk

  Written by thnikk.
*************************************************/

#include <EEPROM.h>
#include <Bounce2.h>
#include "KeyboardioHID.h"

// Version number (increment to update EEPROM values)
bool version = 0;
char initMapping[] = {"sdfjkl "};

// How many keys (0 indexed)
#define numkeys 7

// Array for buttons (for use in for loop.)
const byte button[] = { 2, 3, 4, 5, 6, 7, 8, 10};

// Makes button press/release action happen only once
bool pressed[numkeys];

// Mapping multidimensional array
char mapping[numkeys][3];

unsigned long previousMillis = 0;
byte set = 0;

// Bounce declaration
Bounce * bounce = new Bounce[numkeys+1];

void setup() {
  // Serial.begin(9600);

  // Set input pullup resistors
  for (int x = 0; x <= numkeys; x++) {
   pinMode(button[x], INPUT_PULLUP);
   bounce[x].attach(button[x]);
   bounce[x].interval(8);
  }

  Keyboard.begin();
}

void loop() {

  // Refresh bounce values
  for(byte x=0; x<=numkeys; x++) bounce[x].update();

  keyboard();
}

// Does keyboard stuff
void keyboard(){

  if (!bounce[0].read()) Keyboard.press(HID_KEYBOARD_S_AND_S);
  if (!bounce[1].read()) Keyboard.press(HID_KEYBOARD_D_AND_D);
  if (!bounce[2].read()) Keyboard.press(HID_KEYBOARD_F_AND_F);
  if (!bounce[3].read()) Keyboard.press(HID_KEYBOARD_J_AND_J);
  if (!bounce[4].read()) Keyboard.press(HID_KEYBOARD_K_AND_K);
  if (!bounce[5].read()) Keyboard.press(HID_KEYBOARD_L_AND_L);
  if (!bounce[6].read()) Keyboard.press(HID_KEYBOARD_SPACEBAR);
  if (!bounce[7].read()) Keyboard.press(HID_KEYBOARD_ESCAPE);

  Keyboard.sendReport();
  Keyboard.releaseAll();
}
