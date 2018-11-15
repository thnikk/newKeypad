 /*****************************************************
  New 4K RGB keypad
    Written to replace the old code to revert back
    from RGBW to RGB LEDs. Gateron's transparent body
    switches are no longer available so the revert
    was required to go from 5050 to 3535 LEDs for
    compatibility with Gateron SMD switches.

    It also implements the new button remapper. Since
    the side button has multiple functions on this
    model (LED mode switching, brightness adjustment,
    color adjustment, and escape) there is only one
    page of macros, and the keypad will need to be
    remapped every time you wish to change macros.

  Key Numbering
    -----------------
  - | 1 | 2 | 3 | 4 |
    -----------------
  ^ Side button is key 5
  *Pinout is 2, 3, 4, 5, 7

  For more info, please visit:
  ----> http://thnikk.moe/
  ----> https://www.etsy.com/shop/thnikk
  ----> twitter.com/thnikk
  ----> https://github.com/thnikk

  Written by thnikk.
*************************************************/

#include <EEPROM.h>
#include <Bounce2.h>
// #include <Keyboard.h>
#include "KeyboardioHID.h"
#include <Adafruit_NeoPixel.h>

// Uncomment if you're using an RGBW model
// #define RGBW

// Version number (increment to update EEPROM values)
bool version = 1;
// Inital EEPROM value arrays
byte initColor[] = {50, 100, 150, 200, 250, 50, 100, 150};
char initMapping[] = {"sdfjkl  "};

#define numkeys 7

// Array for buttons (for use in for loop.)
const byte button[] = { 2, 3, 7, 9, 10, 11, 12, 4 };

// Makes button press/release action happen only once
bool pressed[numkeys];

unsigned long previousMillis = 0;
byte set = 0;

// Bounce declaration
Bounce * bounce = new Bounce[numkeys+1];

// Neopixel
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numkeys, 5, NEO_GRB + NEO_KHZ800);

// Multidimensional arrays
char mapping[numkeys][3];
byte rgb[numkeys][3];

// Universal
byte ledMode = 1;
byte numModes = 6;
float brightness = 1.0;

// Cycle LED Mode
unsigned long cycleSpeed = 10;  // Edit to change cycle speed, higher is slower
byte cycleWheel = 0;            // Cycle wheel counter
unsigned long cycleMillis = 0;  // Millis counter

// Reactive LED mode
byte colorState[numkeys];             // States between white, rainbow, fadeout, and off (per key)
byte reactiveWheel[numkeys];          // Reactive wheel counter
unsigned long reactSpeed = 1;
unsigned long reactMillis = 0;

// Custom LED mode
byte customWheel[numkeys];
unsigned long customMillis = 0;
unsigned long customSpeed = 20;

// BPS LED mode
unsigned long bpsMillis = 0;
unsigned long bpsMillis2 = 0;
unsigned long bpsSpeed = 0;
unsigned long bpsUpdate = 1000;
byte bpsCount = 0;
byte bpsBuffer = 170;
byte bpsFix = 170;

// Color moodes
byte colors[] = { 160, 200, 160, 160, 200, 160, 200 };

// Side button
unsigned long s = 500;
unsigned long m = 1500;
unsigned long sideMillis = 0;
unsigned long brightMillis = 0;
byte hold = 0;
byte blink = 0;

byte specialLength = 29; // Number of "special keys"
String specialKeys[] = {
  "shift", "ctrl", "super", "alt",
  "f1", "f2", "f3", "f4",
  "f5", "f6", "f7", "f8",
  "f9", "f10", "f11", "f12",
  "insert", "delete", "backspace", "enter",
  "home", "end", "pgup", "pgdn",
  "up", "down", "left", "right",
  "tab", "altGr"
};
byte specialByte[] = {
  0xE1, 0xE0, 0xE3, 0xE2,
  0x3A, 0x3B, 0x3C, 0x3D,
  0x3E, 0x3F, 0x40, 0x41,
  0x42, 0x43, 0x44, 0x45,
  0x49, 0x4C, 0x2A, 0x28,
  0x4A, 0x4D, 0x4B, 0x4E,
  0x52, 0x51, 0x50, 0x4F,
  0xBA, 0xE6
};

byte inputBuffer; // Stores specialByte after conversionr; // Stores specialByte after conversion

byte hexMap[] = {
  0x2C, 0x2D, 0x2E, 0x2F, // Space, -, =, [
  0x30, 0x31, 0x33, 0x34, // ], \, ;, '
  0x35, 0x36, 0x37, 0x38 // `, <,>, ., /
};

byte byteMap[] = {
  32, 45, 61, 91,
  93, 92, 59, 39,
  96, 60, 62, 47
};

String nameMap[] = {
  "Space", "-", "=", "[",
  "]", "backslash", ";", "'",
  "`", ",", ".", "/"
};

void setup() {
  Serial.begin(9600);
  pixels.begin();
  // Load + Init EEPROM
  loadEEPROM();

  // Set input pullup resistors
  for (int x = 0; x <= numkeys; x++) {
    pinMode(button[x], INPUT_PULLUP);
    bounce[x].attach(button[x]);
    bounce[x].interval(8);
  }
  pinMode(button[numkeys], INPUT_PULLUP);

  Keyboard.begin();
}

byte kToB(byte input) {
  if (input >= 97 && input <=122) return input - 93; // Letters get shifted by -93
  else if (input >= 48 && input <=57 ){ // 1=30 and 0=39
    if (input == 48) return 39;
    else return input - 19;
  }
  else for (byte x=0; x<sizeof(hexMap); x++) {
    if (byteMap[x] == input) return hexMap[x]; // Convert remainder
    if (x == (sizeof(hexMap)-1) && byteMap[x] != input) return input; // Otherwise, leave unconverted (this shouldn't happen anyway)
  }
}

byte bToK(byte input){
  if (input >= 4 && input <=29) return input + 93; // Letters get shifted by -93
  else if (input >= 30 && input <=39 ){ // 1=30 and 0=39
    if (input == 39) return 48;
    else return input + 19;
  }
  else for (byte x=0; x<sizeof(hexMap); x++) if (byteMap[x] == input) return hexMap[x];
}

// Array for buttons (for use in fo
void loadEEPROM() {
  // Initialize EEPROM
  if (EEPROM.read(0) != version) {
    // Single values
    EEPROM.write(0, version); // Update version
    EEPROM.write(20, 0); // Start LED mode on cycle for testing
    EEPROM.write(21, 50);// Start brightness at half
    for (int x = 0; x < numkeys; x++) {
      EEPROM.write(30+x,initColor[x]);
      for (int  y= 0; y < 3; y++) {
        if (y == 0) EEPROM.write(40+(x*3)+y, kToB(int(initMapping[x])));
        if (y > 0) EEPROM.write(40+(x*3)+y, 0);
      }
    }
  }

  // Load values from EEPROM
  for (int x = 0; x < numkeys; x++) {
    // Load button mapping
    for (int  y= 0; y < 3; y++) {
      mapping[x][y] = EEPROM.read(40+(x*3)+y);
    }
    // Load custom wheel values
    customWheel[x] = EEPROM.read(30+x);
  }
  ledMode = EEPROM.read(20);
  brightness = EEPROM.read(21);
  brightness = brightness / 100;
}

void loop() {

  // Refresh bounce values
  for(byte x=0; x<=numkeys; x++) bounce[x].update();

  if ((millis() - previousMillis) > 1000) { // Check once a second to reduce overhead

    if (Serial && set == 0) { // Run once when serial monitor is opened to avoid flooding the serial monitor
      Serial.println("Please press 0 to enter the serial remapper.");
      set = 1;
    }
    // If 0 is received at any point, enter the remapper.
    if (Serial.available() > 0) if (Serial.read() == '0') remapSerial();
    // If the serial monitor is closed, reset so it can prompt the user to press 0 again.
    if (!Serial) set = 0;

    previousMillis = millis();
  }

  sideButton();
  if (ledMode == 0) cycle();
  if (ledMode == 1) reactive(0);
  if (ledMode == 2) reactive(1);
  if (ledMode == 3) custom();
  if (ledMode == 4) BPS();
  // if (ledMode == 5) taiko();
  if (ledMode == 5) colorChange();

  keyboard();
}

void sideButton() {
  // Press action: Sets hold value depending on how long the side button is held
  if (!digitalRead(button[numkeys])) {
    if ((millis() - sideMillis) > 8 && (millis() - sideMillis) < s)  hold = 1;
    if ((millis() - sideMillis) > s && (millis() - sideMillis) < m)  hold = 2;
    if ((millis() - sideMillis) > m) hold = 3;
  }
  // Release action
  if (digitalRead(button[numkeys])) {
    // Press and release escape
    if (hold == 1) {
      Keyboard.press(HID_KEYBOARD_ESCAPE);
      Keyboard.sendReport();
      delay(12);
      Keyboard.release(HID_KEYBOARD_ESCAPE);
      Keyboard.sendReport();
    }
    // Change LED mode
    if (hold == 2) {
      ledMode++;
      for (int x = 0; x < numkeys; x++) for (int y = 0; y < 3; y++) rgb[x][y] = 0; // Clear colors
      if (ledMode > (numModes - 1)) ledMode = 0;
      EEPROM.write(20, ledMode);
    }
    // Save custom colors or brightness
    if (hold == 3) {
      // Save custom colors
      if (ledMode == 3) {
        for (int x = 0; x < numkeys; x++) {
          EEPROM.write(30+x, customWheel[x]);
        }
      }
      // Save brightness to EEPROM
      if (ledMode != 3) {
        for (int x = 0; x < numkeys; x++) {
          EEPROM.write(21, brightness * 100);
        }
      }
    }
    hold = 0;
    sideMillis = millis();
  }

  // Brightness changer
  if (hold == 3 && ledMode != 3) {
    // Poll 10 times a second
    if ((millis() - brightMillis) > 50) {
      // Lower brightness
      if (!bounce[0].read()) {
        if (brightness > 0.1) brightness-=0.02;
      }
      // Raise brightness
      if (!bounce[1].read()) {
        if (brightness < 0.98) brightness+=0.02;
      }
      brightMillis = millis();
    }
  }

  // Blink code
  if (blink != hold) {
    if (hold == 2) {
      for (int x = 0; x < numkeys; x++) setColor(0, x);
      pixels.show();
      delay(20);
      for (int x = 0; x < numkeys; x++) setColor(255, x);
      pixels.show();
      delay(50);
    }
    if (hold == 3) {
      for (int y = 0; y < 2; y++) {
        for (int x = 0; x < numkeys; x++) setColor(0, x);
        pixels.show();
        delay(20);
        for (int x = 0; x < numkeys; x++) setColor(255, x);
        pixels.show();
        delay(50);
      }
    }
    blink = hold;
  }
}

byte cycleAdd = 20;
// LED Modes
void cycle() {
  if ((millis() - cycleMillis) > cycleSpeed) {
    cycleWheel++; // No rollover needed since datatype is byte
    for(int x = 0; x < numkeys-2; x++) {
      wheel(cycleWheel + (cycleAdd*x), x);
      setLED(x);
    }
    wheel(cycleWheel + (cycleAdd*4), numkeys-2);
      setLED(numkeys-2);
    wheel(cycleWheel + (cycleAdd*3), numkeys-1);
      setLED(numkeys-1);
    pixels.show();
    cycleMillis = millis();
  }
}

byte reactFaster = 5;
void reactive(byte flip) {
  if ((millis() - reactMillis) > reactSpeed) {
    for (int a=0; a < numkeys; a++) {
      // Press action
      if ((!bounce[a].read() && !flip) || (bounce[a].read() && flip)) {
        for (int x = 0; x < 3; x++) rgb[a][x] = 255;
        colorState[a] = 1;
      }

      // Release action
      if ((bounce[a].read() && !flip) || (!bounce[a].read() && flip)) {
        // Decrements white and increments red
        if (colorState[a] == 1) {
          for (int x = 1; x < 3; x++) {
            byte buffer = rgb[a][x];
            if (buffer > 0) buffer-=reactFaster;
            rgb[a][x] = buffer;
          }

          if ((rgb[a][2] == 0) && (rgb[a][1] == 0) && (rgb[a][0] == 255)) {
            colorState[a] = 2;
            reactiveWheel[a] = 0;
          }

        }

        if (colorState[a] == 2) {
          wheel(reactiveWheel[a], a);
          byte buffer = reactiveWheel[a];
          buffer+=reactFaster;
          reactiveWheel[a] = buffer;
          if (reactiveWheel[a] == 170) colorState[a] = 3;
        }

        if (colorState[a] == 3) {
          if (rgb[a][2] > 0) {
            byte buffer = rgb[a][2];
            buffer-=reactFaster;
            rgb[a][2] = buffer;
          }
          if (rgb[a][2] == 0) colorState[a] = 0;
        }

        if (colorState[a] == 0) {
          for (int x = 0; x < 3; x++) {
            rgb[a][x] = 0;
          }
        }

      } // End of release
      setLED(a);
    }
    pixels.show();
    reactMillis = millis();
  }
}

void custom() {
  if ((millis() - customMillis) > customSpeed) {
    for (int x = 0; x < numkeys; x++){
      // When side button is held
      if (hold == 3) {
        // When key is pressed
        if (!bounce[x].read()) {
          byte buffer = customWheel[x];
          buffer++; // No rollover needed since datatype is byte
          customWheel[x] = buffer;
        }
      }
      // When a button isn't being held or a color is being changed
      if (bounce[x].read() || hold == 3) {
        wheel(customWheel[x], x);
        setLED(x);
      }
      // LEDs turn white on keypress
      if (!bounce[x].read() && hold != 3) setColor(255, x);
    }
    pixels.show();
    customMillis = millis();
  }

}

void BPS() {
  // Check counter every second, apply multiplier to wheel, and wipe counter
  if ((millis() - bpsMillis) > bpsUpdate) {
    bpsFix = 170-(bpsCount*9);                      // Start at blue and move towards red
    if (bpsFix < 0) bpsFix = 0;                     // Cap color at red
    bpsCount = 0;                                   // Reset counter
    bpsMillis = millis();                           // Reset millis timer
  }
  if ((millis() - bpsMillis2) > 5) {                // Run once every five ms for smooth fades
    if (bpsBuffer < bpsFix) bpsBuffer++;            // Fade up if buffer value is lower
    if (bpsBuffer > bpsFix) bpsBuffer--;            // Fade down if buffer value is higher
    for (int x = 0; x < numkeys; x++) {
      wheel(bpsBuffer, x);                          // convert wheel value to rgb values in array
        setLED(x);
        if (!bounce[x].read()) setColor(255, x);
      }
    pixels.show();                                  // Update LEDs
    bpsMillis2 = millis();                          // Reset secondary millis timer
  }
}

void taiko() {
  for (int x = 0; x < numkeys; x++) {
    wheel(colors[x], x);
    setLED(x);
    if (!bounce[x].read() && hold != 3) setColor(255, x);
  }
  pixels.show();
}

// Subfunctions for LED modes
// Color wheel function (reduced color depth for easier eeprom storage)
void wheel(byte shortColor, byte key) { // Set RGB color with byte
  if (shortColor >= 0 && shortColor < 85) { rgb[key][0] = (shortColor * -3) +255; rgb[key][1] = shortColor * 3; rgb[key][2] = 0; }
  else if (shortColor >= 85 && shortColor < 170) { rgb[key][0] = 0; rgb[key][1] = ((shortColor - 85) * -3) +255; rgb[key][2] = (shortColor - 85) * 3; }
  else { rgb[key][0] = (shortColor - 170) * 3; rgb[key][1] = 0; rgb[key][2] = ((shortColor - 170) * -3) +255; }
}

void setLED(byte key) { pixels.setPixelColor(key, pixels.Color(rgb[key][0] * brightness, rgb[key][1] * brightness, rgb[key][2] * brightness)); }
void setColor(byte color, byte key) { pixels.setPixelColor(key, pixels.Color(color * brightness, color * brightness, color * brightness)); }

// Remapper code
// Allows keys to be remapped through the serial monitor
void remapSerial() {
  Serial.println("Welcome to the serial remapper!");
  // Buffer variables (puting these at the root of the relevant scope to reduce memory overhead)
  byte input = 0;

  // Print current EEPROM values
  Serial.print("Current values are: ");
  for (int x = 0; x < numkeys; x++) {
    for (int y = 0; y < 3; y++) {
      byte mapCheck = int(mapping[x][y]);
      if (mapCheck != 0){ // If not null...
        // Print if regular character (prints as a char)
        if (mapCheck > 33 && mapCheck < 126) Serial.print(mapping[x][y]);
        // Otherwise, check it through the byte array and print the text version of the key.
        else for (int z = 0; z < sizeof(specialByte); z++) if (specialByte[z] == mapCheck){
          Serial.print(specialKeys[z]);
          Serial.print(" ");
        }
      }
    }
    // Print delineation
    if (x < (numkeys - 1)) Serial.print(", ");
  }
  Serial.println();
  // End of print

  // Take serial inputs
  Serial.println("Please input special keys first and then a printable character.");
  Serial.println();
  Serial.println("For special keys, please enter a colon and then the corresponding");
  Serial.println("number (example: ctrl = ':1')");
  // Print all special keys
  byte lineLength = 0;

  // Print table of special values
  for (int y = 0; y < 67; y++) Serial.print("-");
  Serial.println();
  for (int x = 0; x < sizeof(specialByte); x++) {
    // Make every line wrap at 30 characters
    byte spLength = specialKeys[x].length(); // save as variable within for loop for repeated use
    lineLength += spLength + 6;
    Serial.print(specialKeys[x]);
    spLength = 9 - spLength;
    for (spLength; spLength > 0; spLength--) { // Print a space
      Serial.print(" ");
      lineLength++;
    }
    if (x > 9) lineLength++;
    Serial.print(" = ");
    if (x <= 9) {
      Serial.print(" ");
      lineLength+=2;
    }
    Serial.print(x);
    if (x != sizeof(specialByte)) Serial.print(" | ");
    if (lineLength > 55) {
      lineLength = 0;
      Serial.println();
    }
  }
  // Bottom line
  if ((sizeof(specialByte) % 4) != 0) Serial.println(); // Add a new line if table doesn't go to end
  for (int y = 0; y < 67; y++) Serial.print("-"); // Bottom line of table
  Serial.println();
  Serial.println("If you want two or fewer modifiers for a key and");
  Serial.println("no printable characters, finish by entering 'xx'");
  // End of table

  for (int x = 0; x < numkeys; x++) { // Main for loop for each key

    byte y = 0; // External loop counter for while loop
    byte z = 0; // quickfix for bug causing wrong input slots to be saved
    while (true) {
      while(!Serial.available()){}
      String serialInput = Serial.readString();
      byte loopV = inputInterpreter(serialInput);
      serialInput[0] = kToB(serialInput[0]);

      // If key isn't converted
      if (loopV == 0){ // Save to array and EEPROM and quit; do and break
        // If user finishes key
        if (serialInput[0] == 'x' && serialInput[1] == 'x') { // Break if they use the safe word
          for (y; y < 3; y++) { // Overwrite with null values (0 char = null)
            EEPROM.write((40+(x*3)+y), 0);
            mapping[x][y] = 0;
          }
          if (x < numkeys-1) Serial.print("(finished key,) ");
          if (x == numkeys-1) Serial.print("(finished key)");
          break;
        }
        // If user otherwise finishes inputs
        Serial.print(serialInput); // Print once
        if (x < 5) Serial.print(", ");
        for (y; y < 3; y++) { // Normal write/finish
          EEPROM.write((40+(x*3)+y), serialInput[y-z]);
          mapping[x][y] = serialInput[y-z];
        }
        break;
      }

      // If key is converted
      if (loopV == 1){ // save input buffer into slot and take another serial input; y++ and loop
        EEPROM.write((40+(x*3)+y), inputBuffer);
        mapping[x][y] = inputBuffer;
        y++;
        z++;
      }

      // If user input is invalid, print keys again.
      if (loopV == 2){
        for (int a = 0; a < x; a++) {
          for (int d = 0; d < 3; d++) {
            byte mapCheck = int(mapping[a][d]);
            if (mapCheck != 0){ // If not null...
              // Print if regular character (prints as a char)
              if (mapCheck > 33 && mapCheck < 126) Serial.print(mapping[a][d]);
              // Otherwise, check it through the byte array and print the text version of the key.
              else for (int c = 0; c < sizeof(specialByte); c++) if (specialByte[c] == mapCheck){
                Serial.print(specialKeys[c]);
                // Serial.print(" ");
              }
            }
          }
          // Print delineation
          Serial.print(", ");
        }
        if (y > 0) { // Run through rest of current key if any inputs were already entered
          for (int d = 0; d < y; d++) {
            byte mapCheck = int(mapping[x][d]);
            if (mapCheck != 0){ // If not null...
              // Print if regular character (prints as a char)
              if (mapCheck > 33 && mapCheck < 126) Serial.print(mapping[x][d]);
              // Otherwise, check it through the byte array and print the text version of the key.
              else for (int c = 0; c < sizeof(specialByte); c++) if (specialByte[c] == mapCheck){
                Serial.print(specialKeys[c]);
                Serial.print(" ");
              }
            }
          }
        }

      }
    } // Mapping loop
  } // Key for loop
  Serial.println();
  Serial.println("Mapping saved, exiting. To re-enter the remapper, please enter 0.");

} // Remapper loop

byte inputInterpreter(String input) { // Checks inputs for a preceding colon and converts said input to byte
  if (input[0] == ':') { // Check if user input special characters
    input.remove(0, 1); // Remove colon
    int inputInt = input.toInt(); // Convert to integer
    if (inputInt >= 0 && inputInt < sizeof(specialByte)) { // Checks to make sure length matches
      inputBuffer = specialByte[inputInt];
      Serial.print(specialKeys[inputInt]); // Print within function for easier access
      Serial.print(" "); // Space for padding
      return 1;
    }
    Serial.println();
    Serial.println("Invalid code added, please try again.");
    return 2;
  }
  else if (input[0] != ':' && input.length() > 3){
    Serial.println();
    Serial.println("Invalid, please try again.");
    return 2;
  }
  else return 0;
}

// Does keyboard stuff
void keyboard(){

  // Test bounce code
  for (int a = 0; a < numkeys; a++) {
    // Cycles through key and modifiers set
    if (!pressed[a]) {
      // For BPS LED mode
      if (ledMode == 4 && !bounce[a].read()) bpsCount++;
      // Runs 3 times for each key
      for (int b = 0; b < 3; b++) if (!bounce[a].read() && hold != 3) {
        pressed[a] = 1;
      }
    }
    if (pressed[a]) {
      for (int b = 0; b < 3; b++) if (bounce[a].read() || hold == 3) {
        pressed[a] = 0;
      }
    }

  }

  for (byte x=0; x<numkeys; x++){
    if (!bounce[x].read()) Keyboard.press(mapping[x][0]);
  }

  Keyboard.sendReport();
  Keyboard.releaseAll();

}

// New LED modes can be added to the bottom along with assoicaiated variables.
// When adding new modes, make sure to increment numModes and add the new
// function to the main loop.

// Color Change: cycles through colors per key when key is pressed.
// This could also be done without the press lock and with a simple
// incrementor and millis timer for a smooth fade through colors when
// the button is held, but I like this effect.
// Idea from WubWoofBacon (https://osu.ppy.sh/u/5750193)
bool pressedCC[numkeys]; // New pressed array for colorChange
byte changeColors[numkeys]; // Array for storing colors; auto rollover since it's a byte
byte changeVal = 17; // Amount to increment color on keypress
unsigned long changeMillis; // For millis timer
void colorChange(){
  for (byte a = 0; a < numkeys; a++) {
    if (!pressedCC[a]) {
      if (!bounce[a].read()) {
        changeColors[a] = changeColors[a] += changeVal;
        pressedCC[a] = 1;
      }
    }
    if (pressedCC[a]) if (bounce[a].read())pressedCC[a] = 0;
    wheel(changeColors[a], a);
  }

  if ((millis() - changeMillis) > 10) { // Limit changes to once ber 10 ms for reduced overhead
    for (byte x=0; x < numkeys; x++) setLED(x);
    pixels.show();
    changeMillis = millis();
  }

}
