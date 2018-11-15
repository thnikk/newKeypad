/*****************************************************
  LED Keypad
    New LED code that implements the new button
    remapper. Since the side button has multiple
    functions on this model (LED mode switching,
    brightness adjustment, color adjustment, and
    escape) there is only one page of macros, and
    the keypad will need to be remapped every time
    you wish to change macros.

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

// Version number (change to update EEPROM values)
bool version = 1;
char initMapping[] = {"zx"};

// How many keys (0 indexed)
const byte numkeys = 2;

// Array for buttons (for use in for loop.)
const byte button[] = { 2, 3, 4 };

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

// Mapping multidimensional array
char mapping[2][3];

// LED array
byte led[2];
byte ledPin[2] = {5, 6};

// Universal
byte ledMode = 1;
float b = 1.0;
unsigned long limitMillis = 0;
byte ledMax = 100;

// LED modes
bool breatheFlip = 0;
byte breatheVal = 0;

// Side button
unsigned long s = 500;
unsigned long m = 1500;
unsigned long sideMillis = 0;
unsigned long brightMillis = 0;
byte hold = 0;
byte blink = 0;

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


void setup() {
  Serial.begin(9600);
  // Load + Init EEPROM
  loadEEPROM();

  // Set input pullup resistors
  for (int x = 0; x < 3; x++) {
   pinMode(button[x], INPUT_PULLUP);
  }

  // Set LEDs as outputs
  for (int x = 0; x < numkeys; x++) pinMode(ledPin[x], OUTPUT);

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
    // Single values
    EEPROM.write(0, version);
    EEPROM.write(20, 0); // Start LED mode on cycle for testing
    EEPROM.write(21, 50);// Start b at half
    for (int x = 0; x < numkeys; x++) {
      for (int  y= 0; y < 3; y++) {
        if (y == 0) EEPROM.write(40+(x*3)+y, int(initMapping[x]));
        if (y > 0) EEPROM.write(40+(x*3)+y, 0);
      }
    }
  }
  // Load values from EEPROM
  for (int x = 0; x < numkeys; x++) for (int  y= 0; y < 3; y++) mapping[x][y] = char(EEPROM.read(40+(x*3)+y));
  ledMode = EEPROM.read(20);
  b = EEPROM.read(21);
  b = b / 100;
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

  if (ledMode == 0) reactive(0);
  if (ledMode == 1) reactive(1);
  if (ledMode == 2) breathe();
  if (ledMode == 3) for(int x = 0; x < numkeys; x++) if (hold != 3) setColor(ledMax, x);
  if (ledMode == 4) for(int x = 0; x < numkeys; x++) if (hold != 3) setColor(0, x);

  sideButton();
  keyboard();
}

void reactive(byte flip) {
  if ((millis() - limitMillis) > 2) {
    for (int x = 0; x < numkeys; x++){
      if ((!bounce[x] && !flip) || (bounce[x] && flip)) led[x] = ledMax;
      if ((bounce[x] && !flip) || (!bounce[x] && flip)) {
        if (led[x] > 0) {
          byte buffer = led[x];
          buffer--;
          led[x] = buffer;
        }
      }
      setLED(x);
    }
    limitMillis = millis();
  }
}

void breathe() {
  if ((millis() - limitMillis) > 20) {
    if (!breatheFlip) {
      breatheVal++;
      if (breatheVal == ledMax) breatheFlip = 1;
    }
    if (breatheFlip) {
      breatheVal--;
      if (breatheVal == 0) breatheFlip = 0;
    }
    for (int x = 0; x < numkeys; x++) setColor(breatheVal, x);
    limitMillis = millis();
  }
}

// LED modes

// Subfunctions for LED modes
void setLED(byte key) {
  analogWrite(ledPin[key], led[key] * b);
}

void setColor(byte color, byte key) {
  analogWrite(ledPin[key], color * b);
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

void sideButton() {
  // Press action: Sets hold value depending on how long the side button is held
  if (!bounce[2]) {
    if ((millis() - sideMillis) > 8 && (millis() - sideMillis) < s)  hold = 1;
    if ((millis() - sideMillis) > s && (millis() - sideMillis) < m)  hold = 2;
    if ((millis() - sideMillis) > m && (ledMode != 4)) hold = 3; // Don't run on Off mode
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
      if (ledMode > 4) ledMode = 0;
      EEPROM.write(20, ledMode);
    }
    // Save brightness
    if (hold == 3) {
      if (ledMode != 3) {
        for (int x = 0; x < numkeys; x++) {
          EEPROM.write(21, b * 100);
        }
      }
    }
    hold = 0;
    sideMillis = millis();
  }

  // brightness changer
  if (hold == 3 && ledMode != 3) {
    // Poll 10 times a second
    if ((millis() - brightMillis) > 50) {
      // Lower b
      if (!bounce[0]) {
        if (b > 0.1) b-=0.02;
      }
      // Raise b
      if (!bounce[1]) {
        if (b < 0.98) b+=0.02;
      }
      brightMillis = millis();
    }
  }

  // Blink code
  if (blink != hold) {
    if (hold == 2) {
      for (int x = 0; x < 2; x++) setColor(0, x);
      delay(20);
      for (int x = 0; x < 2; x++) setColor(ledMax, x);
      delay(50);
    }
    if (hold == 3) {
      for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) setColor(0, x);
        delay(20);
        for (int x = 0; x < 2; x++) setColor(ledMax, x);
        delay(50);
      }
    }
    blink = hold;
  }
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
