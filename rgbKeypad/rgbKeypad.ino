/*****************************************************
  New RGB keypad
    Written to replace the old code to revert back
    from RGBW to RGB LEDs. Gateron's transparent body
    switches are no longer available so the revert
    was required to go from 5050 to 3535 LEDs for
    compatibility with Gateron SMD switches.

    It also implements the new button remapper. Since
    the side button has multiple functions on this
    model (LED mode switching, b adjustment,
    color adjustment, and escape) there is only one
    page of macros, and the keypad will need to be
    remapped every time you wish to change macros.

  Key Numbering
    ---------
  - | 1 | 2 |
    ---------
  ^ Side button is key 3
  *Pinout is 2, 4, 5 for buttons 1, 2, and 3

  For more info, please visit:
  ----> http://thnikk.moe/
  ----> https://www.etsy.com/shop/thnikk
  ----> twitter.com/thnikk
  ----> https://github.com/thnikk

  Written by thnikk.
*************************************************/

#include <EEPROM.h>
#include <Bounce2.h>
#include <Keyboard.h>
#include <Adafruit_NeoPixel.h>

// Uncomment if you're using an RGBW model
// #define RGBW

// How many keys (0 indexed)
const byte numkeys = 2;

// Array for buttons (for use in for loop.)
const byte button[] = { 2, 4, 5 };

// Makes button press/release action happen only once
bool pressed[2];

// Array for storing bounce values
bool bounce[3];

unsigned long previousMillis = 0;
byte set = 0;

// Bounce declaration
Bounce b1d = Bounce();
Bounce b2d = Bounce();
Bounce b3d = Bounce();

// Neopixel
#ifdef RGBW
  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numkeys, 3, NEO_GRBW + NEO_KHZ800);
#else
  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numkeys, 3, NEO_GRB + NEO_KHZ800);
#endif

// Arrays for modifier interpreter
byte specialLength = 29; // Number of "special keys"
String specialKeys[] = {
  "shift", "ctrl", "super",
  "alt", "f1", "f2", "f3",
  "f4", "f5", "f6", "f7",
  "f8", "f9", "f10", "f11",
  "f12", "insert",
  "delete", "backspace",
  "enter", "home", "end",
  "pgup", "pgdn", "up",
  "down", "left", "right",
  "tab"
};
byte specialByte[] = {
  129, 128, 131, 130,
  194, 195, 196, 197,
  198, 199, 200, 201,
  202, 203, 204, 205,
  209, 212, 178, 176,
  210, 213, 211, 214,
  218, 217, 216, 215,
  179
};

byte inputBuffer; // Stores specialByte after conversion

// Multidimensional arrays
char mapping[2][3];
byte rgb[2][3];

// Version number (change to update EEPROM values {can only be 0 or 1})
bool version = 1;

// Universal
byte ledMode = 0;
byte numModes = 6;
byte b = 127;  // Brightness

// Cycle LED Mode
unsigned long cycleSpeed = 10;  // Edit to change cycle speed, higher is slower
byte cycleWheel = 0;            // Cycle wheel counter
unsigned long cycleMillis = 0;  // Millis counter

// Reactive LED mode
byte colorState[2];             // States between white, rainbow, fadeout, and off (per key)
byte reactiveWheel[2];          // Reactive wheel counter
unsigned long reactSpeed = 0;
unsigned long reactMillis = 0;

// Custom LED mode
byte customWheel[2];
unsigned long customMillis = 0;
unsigned long customSpeed = 5;

// BPS LED mode
unsigned long bpsMillis = 0;
unsigned long bpsMillis2 = 0;
unsigned long bpsSpeed = 0;
unsigned long bpsUpdate = 1000;
byte bpsCount = 0;
byte bpsBuffer = 170;
byte bpsFix = 170;

// Side button
unsigned long s = 500;
unsigned long m = 1500;
unsigned long sideMillis = 0;
unsigned long brightMillis = 0;
byte hold = 0;
byte blink = 0;

void setup() {
  Serial.begin(9600);
  pixels.begin();
  // Load + Init EEPROM
  loadEEPROM();

  // Set input pullup resistors
  for (int x = 0; x < 3; x++) {
   pinMode(button[x], INPUT_PULLUP);
  }

  // Bounce initializtion
  b1d.attach(button[0]);
  b1d.interval(8);
  b2d.attach(button[1]);
  b2d.interval(8);
  b3d.attach(button[2]);
  b3d.interval(8);
}

void loadEEPROM() {
  // Initialize EEPROM
  if (EEPROM.read(0) != version) {
    // Inital EEPROM value arrays go here to not waste system memory
    byte initColor[] = {100, 200};
    char initMapping[] = {"zx"};

    // Single values
    EEPROM.write(0, version);
    EEPROM.write(20, ledMode); // Start LED mode on cycle for testing
    EEPROM.write(21, b);// Start brightness at half
    for (int x = 0; x < numkeys; x++) {
      EEPROM.write(30+x,initColor[x]);
      for (int  y= 0; y < 3; y++) {
        if (y == 0) EEPROM.write(40+(x*3)+y, int(initMapping[x]));
        if (y > 0) EEPROM.write(40+(x*3)+y, 0);
      }
    }
  }

  // Load EEPROM
  for (int x = 0; x < numkeys; x++) {
    // Load button mapping
    for (int  y= 0; y < 3; y++) {
      mapping[x][y] = char(EEPROM.read(40+(x*3)+y));
    }
    // Load custom wheel values
    customWheel[x] = EEPROM.read(30+x);
  }
  ledMode = EEPROM.read(20);
  b = EEPROM.read(21);
}

void loop() {

  // Run to get latest bounce values
  bounceSetup(); // Moved here for program-wide access to latest debounced button values

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
  if (ledMode == 5) colorChange();

  keyboard();
}

void sideButton() {
  // Press action: Sets hold value depending on how long the side button is held
  if (!bounce[2]) {
    if ((millis() - sideMillis) > 8 && (millis() - sideMillis) < s)  hold = 1;
    if ((millis() - sideMillis) > s && (millis() - sideMillis) < m)  hold = 2;
    if ((millis() - sideMillis) > m) hold = 3;
  }
  // Release action
  if (bounce[2]) {
    // Press and release escape
    if (hold == 1) {
      Keyboard.press(KEY_ESC);
      delay(12);
      Keyboard.release(KEY_ESC);
    }
    // Change LED mode
    if (hold == 2) {
      ledMode++;
      for (int x = 0; x < 2; x++) for (int y = 0; y < 3; y++) rgb[x][y] = 0; // Clear colors
      if (ledMode > (numModes-1)) ledMode = 0; // numModes -1 for 0 index
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
      // Save b to EEPROM
      if (ledMode != 3) {
        for (int x = 0; x < numkeys; x++) {
          EEPROM.write(21, b);
        }
      }
    }
    hold = 0;
    sideMillis = millis();
  }

  // brightness changer
  if (hold == 3 && ledMode != 3) {
    // Poll 10 times a second
    if ((millis() - brightMillis) > 10) {
      // Lower brightness
      if (!bounce[0]) {
        if (b > 10) b--;
      }
      // Raise brightness
      if (!bounce[1]) {
        if (b < 255) b++;
      }
      brightMillis = millis();
    }
  }

  // Blink code
  if (blink != hold) {
    if (hold == 2) {
      for (int x = 0; x < 2; x++) setColor(0, x);
      pixels.show();
      delay(20);
      for (int x = 0; x < 2; x++) setColor(255, x);
      pixels.show();
      delay(50);
    }
    if (hold == 3) {
      for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) setColor(0, x);
        pixels.show();
        delay(20);
        for (int x = 0; x < 2; x++) setColor(255, x);
        pixels.show();
        delay(50);
      }
    }
    blink = hold;
  }
}

// LED Modes
void cycle() {
  if ((millis() - cycleMillis) > cycleSpeed) {
    cycleWheel++; // No rollover needed since datatype is byte
    for(int x = 0; x < numkeys; x++) {
      wheel(cycleWheel, x);
      setLED(x);
    }
    pixels.show();
    cycleMillis = millis();
  }
}

void reactive(byte flip) {
  if ((millis() - reactMillis) > reactSpeed) {
    for (int a=0; a < numkeys; a++) {
      // Press action
      if ((!bounce[a] && !flip) || (bounce[a] && flip)) {
        for (int x = 0; x < 3; x++) rgb[a][x] = 255;
        colorState[a] = 1;
      }

      // Release action
      if ((bounce[a] && !flip) || (!bounce[a] && flip)) {
        // Decrements white and increments red
        if (colorState[a] == 1) {
          for (int x = 1; x < 3; x++) {
            byte buffer = rgb[a][x];
            if (buffer > 0) buffer--;
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
          buffer++;
          reactiveWheel[a] = buffer;
          if (reactiveWheel[a] == 170) colorState[a] = 3;
        }

        if (colorState[a] == 3) {
          if (rgb[a][2] > 0) {
            byte buffer = rgb[a][2];
            buffer--;
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
    for (int x = 0; x < 2; x++){
      // When side button is held
      if (hold == 3) {
        // When key is pressed
        if (!bounce[x]) {
          byte buffer = customWheel[x];
          buffer++; // No rollover needed since datatype is byte
          customWheel[x] = buffer;
        }
      }
      // When a button isn't being held or a color is being changed
      if (bounce[x] || hold == 3) {
        wheel(customWheel[x], x);
        setLED(x);
      }
      // LEDs turn white on keypress
      if (!bounce[x] && hold != 3) setColor(255, x);
    }
    pixels.show();
    customMillis = millis();
  }

}

void BPS() {
  // Check counter every second, apply multiplier to wheel, and wipe counter
  if ((millis() - bpsMillis) > bpsUpdate) {
    bpsFix = 170-(bpsCount*17);                     // Start at blue and move towards red
    if (bpsFix < 0) bpsFix = 0;                     // Cap color at red
    bpsCount = 0;                                   // Reset counter
    bpsMillis = millis();                           // Reset millis timer
  }
  if ((millis() - bpsMillis2) > 5) {                // Run once every five ms for smooth fades
    if (bpsBuffer < bpsFix) bpsBuffer++;            // Fade up if buffer value is lower
    if (bpsBuffer > bpsFix) bpsBuffer--;            // Fade down if buffer value is higher
    for (int x = 0; x < 2; x++) {
      wheel(bpsBuffer, x);                          // convert wheel value to rgb values in array
        setLED(x);
        if (!bounce[x]) setColor(255, x);
      }
    pixels.show();                                  // Update LEDs
    bpsMillis2 = millis();                          // Reset secondary millis timer
  }
}

// Subfunctions for LED modes
// Color wheel function (reduced color depth for easier eeprom storage)
void wheel(byte shortColor, byte key) {
  // convert shortColor to r, g, or b
  if (shortColor >= 0 && shortColor < 85) {
    rgb[key][0] = (shortColor * -3) +255;
    rgb[key][1] = shortColor * 3;
    rgb[key][2] = 0;
  }
  if (shortColor >= 85 && shortColor < 170) {
    rgb[key][0] = 0;
    rgb[key][1] = ((shortColor - 85) * -3) +255;
    rgb[key][2] = (shortColor - 85) * 3;
  }
  if (shortColor >= 170 && shortColor < 255) {
    rgb[key][0] = (shortColor - 170) * 3;
    rgb[key][1] = 0;
    rgb[key][2] = ((shortColor - 170) * -3) +255;
  }
}

void setLED(byte key) {
  #ifdef RGBW
    pixels.setPixelColor(key, pixels.Color(b*rgb[key][0]/255, b*rgb[key][1]/255, b*rgb[key][2]/255, 0));
  #else
    pixels.setPixelColor(key, pixels.Color(b*rgb[key][0]/255, b*rgb[key][1]/255, b*rgb[key][2]/255));
  #endif
}

void setColor(byte color, byte key) {
  #ifdef RGBW
    pixels.setPixelColor(key, pixels.Color(b*color/255, b*color/255, b*color/255, 0));
  #else
    pixels.setPixelColor(key, pixels.Color(b*color/255, b*color/255, b*color/255));
  #endif
}


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
        else for (int z = 0; z < specialLength; z++) if (specialByte[z] == mapCheck){
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
  for (int x = 0; x < specialLength; x++) {
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
    if (x != specialLength) Serial.print(" | ");
    if (lineLength > 55) {
      lineLength = 0;
      Serial.println();
    }
  }
  // Bottom line
  if ((specialLength % 4) != 0) Serial.println(); // Add a new line if table doesn't go to end
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
          EEPROM.write((40+(x*3)+y), int(serialInput[y-z]));
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
              else for (int c = 0; c < specialLength; c++) if (specialByte[c] == mapCheck){
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
              else for (int c = 0; c < specialLength; c++) if (specialByte[c] == mapCheck){
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
    if (inputInt >= 0 && inputInt < specialLength) { // Checks to make sure length matches
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

// Sets up bounce inputs and sets values for the bounce array
void bounceSetup() {
  b1d.update();
  b2d.update();
  b3d.update();

  bounce[0] = b1d.read();
  bounce[1] = b2d.read();
  bounce[2] = b3d.read();
}

// Does keyboard stuff
void keyboard(){

  // Test bounce code
  for (int a = 0; a < numkeys; a++) {
    // Cycles through key and modifiers set
    if (!pressed[a]) {
      // For BPS LED mode
      if (ledMode == 4 && !bounce[a]) bpsCount++;
      // Runs 3 times for each key
      for (int b = 0; b < 3; b++) if (!bounce[a] && hold != 3) {
        Keyboard.press(mapping[a][b]);
        pressed[a] = 1;
      }
    }
    if (pressed[a]) {
      for (int b = 0; b < 3; b++) if (bounce[a] || hold == 3) {
        Keyboard.release(mapping[a][b]);
        pressed[a] = 0;
      }
    }

  }
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
      if (!bounce[a]) {
        changeColors[a] = changeColors[a] += changeVal;
        pressedCC[a] = 1;
      }
    }
    if (pressedCC[a]) if (bounce[a])pressedCC[a] = 0;
    wheel(changeColors[a], a);
  }

  if ((millis() - changeMillis) > 10) { // Limit changes to once ber 10 ms for reduced overhead
    for (byte x=0; x < numkeys; x++) setLED(x);
    pixels.show();
    changeMillis = millis();
  }

}

// LED mode for use when remapper is active and keypad is waiting for user input
byte colorCount;
unsigned long idleMillis;
unsigned long idleSpeed = 1; // Speed of effect (lower is faster)
byte idleColors[numkeys][3];
void idleEffect(){
  if ((millis() - idleMillis) > idleSpeed) {
    
  }
}
