/*************************************************
  Macro Keypad
    The first keypad in the line that adds the NEW
    new and improved serial remapper. This will be
    ported to the other models, but the "page"
    functionality will remain specific to this
    model due to input constraints. The updated
    remapper now allows you to map (almost) any
    keyboard character (up to three per key) and
    allows you to switch between six pages by
    holding the side button and pressing the face
    button to the corresponding page.

  Key Numbering
    -------------
    | 1 | 2 | 3 |
    -------------
  - | 4 | 5 | 6 |
    -------------
  ^ Side button is key 7
  Pinout can be seen in the button function

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
#include <Mouse.h>
#include <Adafruit_NeoPixel.h>

bool serialDebug = 0;

// Version number (chnge to update EEPROM values)
bool version = 0;

// First-time mapping values. Update version to write
// new values to EEPROM. Values available at
// ASCII table here: http://www.asciitable.com/
byte byteMapping[6][6] = {
  { 97, 115, 100, 122, 120, 99 }, // asdzxc (osu)
  { 113, 119, 101, 97, 115, 100 }, // qweasd (FPS)
  { 0, 218, 0, 216, 217, 215 }, // arrow keys
  { 49, 50, 51, 52, 53, 54 }, // 123456
  { 0, 0, 0, 0, 0, 0 }, // Blank
  { 0, 0, 0, 0, 0, 0 }  // Blank
};

// Arrays for modifier interpreter
byte specialLength = 33; // Number of "special keys"
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
  "tab", "escape", "MB1", 
  "MB2", "MB3"
};
byte specialByte[] = {
  129, 128, 131, 130,
  194, 195, 196, 197,
  198, 199, 200, 201,
  202, 203, 204, 205,
  209, 212, 178, 176,
  210, 213, 211, 214,
  218, 217, 216, 215,
  179, 177, 1, 2, 3
};

byte inputBuffer; // Stores specialByte after conversion

byte b = 255; // LED brightness

// MacroPad Specific

// Array for buttons (for use in for loop.)
const byte button[] = { 2, 3, 4, 5, 6, 7, 8 };

// How many keys (0 indexed)
#define numkeys 6 // Use define for compiler level
const byte pages = 6;
byte page = 0; // Default page

// Makes button press/release action happen only once
bool pressed[numkeys];

// Array for storing bounce values
bool bounce[numkeys+1]; // +1 because length is NOT zero indexed

// Mapping multidimensional array
char mapping[pages][numkeys][3];

// Neopixel library initializtion
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, 9, NEO_GRB + NEO_KHZ800);
byte rgb[3][1]; // Declare RGB array

unsigned long previousMillis = 0; // Serial monitor timer
unsigned long sbMillis = 0;       // Side button timer
unsigned long effectMillis = 0;   // LED effect timer (so y'all don't get seizures)
byte hold = 0; // Hold counter
byte set = 0;  //
byte rb = 0; // rainbow effect counter (rollover at 255 to reset)
bool blink = 0;

// Bounce declaration
Bounce b1d = Bounce();
Bounce b2d = Bounce();
Bounce b3d = Bounce();
Bounce b4d = Bounce();
Bounce b5d = Bounce();
Bounce b6d = Bounce();
Bounce b7d = Bounce();

void setup() {
  Serial.begin(9600);
  pixels.begin();
  // Load + Initialize EEPROM
  loadEEPROM();

  // Set input pullup resistors
  for (int x = 0; x <= numkeys; x++) {
   pinMode(button[x], INPUT_PULLUP);
  }

  // Bounce initializtion
  b1d.attach(button[0]);
  b1d.interval(8);
  b2d.attach(button[1]);
  b2d.interval(8);
  b3d.attach(button[2]);
  b3d.interval(8);
  b4d.attach(button[3]);
  b4d.interval(8);
  b5d.attach(button[4]);
  b5d.interval(8);
  b6d.attach(button[5]);
  b6d.interval(8);
  b7d.attach(button[6]);
  b7d.interval(8);
}

void loadEEPROM() {
  // Initialize EEPROM
  if (EEPROM.read(0) != version) {
    EEPROM.write(1, page);
    // Single values
    EEPROM.write(0, version);
    for (int z = 0; z < pages; z++) {     // Pages
      for (int x = 0; x < numkeys; x++) { // Keys
        for (int  y= 0; y < 3; y++) {     // 3 key slots per key
          // Write the default values (no combos to avoid accidents)
          if (y == 0) EEPROM.write(((40+(x*3)+y) + (z*3*numkeys)), byteMapping[z][x]);
          // Write 0 to remailing two slots per key for null values
          if (y > 0) EEPROM.write((40+(x*3)+y) + (z*3*numkeys), 0);
        }
      }
    }
  }
  // Load values from EEPROM
  page = EEPROM.read(1);
  // Load button mapping
  for (int z = 0; z < pages; z++) {     // Pages
    for (int x = 0; x < numkeys; x++) { // Keys
      for (int  y= 0; y < 3; y++) {     // 3 key slots per key
        mapping[z][x][y] = char(EEPROM.read((40+(x*3)+y) + (z*3*numkeys)));
      }
    }
  }
}

void loop() {

  // Run to get latest bounce values
  bounceSetup(); // Moved here for program-wide access to latest debounced button values

  if (((millis() - previousMillis) > 1000) && (!serialDebug)) { // Check once a second to reduce overhead
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

  keyboard();
  status(page);
}

// LED code
// Three modes along with the "wheel" function, which lets you set the color between r, g, and b with a single byte (0-255)
void status(byte inPage) { // Displays single color
  if ((millis() - effectMillis) > 10) {
    wheel(inPage*40);
    pixels.setPixelColor(0, pixels.Color(b*rgb[0][0]/255, b*rgb[1][0]/255, b*rgb[2][0]/255));
    pixels.show();
    effectMillis = millis();
  }
}
void rainbow() { // Rainbow effect while waiting for page input in remapper.
  if ((millis() - effectMillis) > 10) { // Using same timer since both will never run at the same time.
    wheel(rb++);
    pixels.setPixelColor(0, pixels.Color(b*rgb[0][0]/255, b*rgb[1][0]/255, b*rgb[2][0]/255));
    pixels.show();
    effectMillis = millis();
  }
}
void blinkLED() { // Blinks LEDs once
  pixels.setPixelColor(0, pixels.Color(b*100/255, b*100/255, b*100/255));
  pixels.show();
  delay(50);
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
  delay(20);
}
void wheel(byte shortColor) { // Set RGB color with byte
  // convert shortColor to r, g, or b
  if (shortColor >= 0 && shortColor < 85) {
    rgb[0][0] = (shortColor * -3) +255;
    rgb[1][0] = shortColor * 3;
    rgb[2][0] = 0;
  }
  if (shortColor >= 85 && shortColor < 170) {
    rgb[0][0] = 0;
    rgb[1][0] = ((shortColor - 85) * -3) +255;
    rgb[2][0] = (shortColor - 85) * 3;
  }
  if (shortColor >= 170 && shortColor < 255) {
    rgb[0][0] = (shortColor - 170) * 3;
    rgb[1][0] = 0;
    rgb[2][0] = ((shortColor - 170) * -3) +255;
  }
}

// Remapper code
// Allows keys to be remapped through the serial monitor
void remapSerial() {
  Serial.println("Welcome to the serial remapper!");
  while(true){
    // Buffer variables (puting these at the root of the relevant scope to reduce memory overhead)
    byte input = 0;
    byte pageMap = 0;

    // Main menu
    Serial.println();
    Serial.println("Please enter the page you'd like to change the mapping for:");
    Serial.println("0 = Exit, 1-6 = Page 1-6");
    while(!Serial.available()){rainbow();} // Display rainbow on LED when waiting for page input
    // Set page or quit
    if (Serial.available()) input = byte(Serial.read()); // Save as variable to reduce overhead
    if (input == 48) break; // Quit if user inputs 0
    else if (input > 48 && input <= 48 + pages) pageMap = input - 49; // -49 for zero indexed mapping array

    // Print current EEPROM values
    Serial.print("Current values are: ");
    for (int x = 0; x < numkeys; x++) {
      for (int y = 0; y < 3; y++) {
        byte mapCheck = int(mapping[pageMap][x][y]);
        if (mapCheck != 0){ // If not null...
          // Print if regular character (prints as a char)
          if (mapCheck > 33 && mapCheck < 126) Serial.print(mapping[pageMap][x][y]);
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
        while(!Serial.available()){status(pageMap);}
        String serialInput = Serial.readString();
        byte loopV = inputInterpreter(serialInput);

        // If key isn't converted
        if (loopV == 0){ // Save to array and EEPROM and quit; do and break
          // If user finishes key
          if (serialInput[0] == 'x' && serialInput[1] == 'x') { // Break if they use the safe word
            for (y; y < 3; y++) { // Overwrite with null values (0 char = null)
              EEPROM.write((40+(x*3)+y) + (pageMap*3*numkeys), 0);
              mapping[pageMap][x][y] = 0;
            }
            if (x < numkeys-1) Serial.print("(finished key,) ");
            if (x == numkeys-1) Serial.print("(finished key)");
            break;
          }
          // If user otherwise finishes inputs
          Serial.print(serialInput); // Print once
          if (x < 5) Serial.print(", ");
          for (y; y < 3; y++) { // Normal write/finish
            EEPROM.write((40+(x*3)+y) + (pageMap*3*numkeys), int(serialInput[y-z]));
            mapping[pageMap][x][y] = serialInput[y-z];
          }
          break;
        }

        // If key is converted
        if (loopV == 1){ // save input buffer into slot and take another serial input; y++ and loop
          EEPROM.write((40+(x*3)+y) + (pageMap*3*numkeys), inputBuffer);
          mapping[pageMap][x][y] = inputBuffer;
          y++;
          z++;
        }

        // If user input is invalid, print keys again.
        if (loopV == 2){

          for (int a = 0; a < x; a++) {
            for (int d = 0; d < 3; d++) {
              byte mapCheck = int(mapping[pageMap][a][d]);
              if (mapCheck != 0){ // If not null...
                // Print if regular character (prints as a char)
                if (mapCheck > 33 && mapCheck < 126) Serial.print(mapping[pageMap][a][d]);
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
              byte mapCheck = int(mapping[pageMap][x][d]);
              if (mapCheck != 0){ // If not null...
                // Print if regular character (prints as a char)
                if (mapCheck > 33 && mapCheck < 126) Serial.print(mapping[pageMap][x][d]);
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
    Serial.println("Mapping saved!");
  } // Main while loop
  Serial.println("Exiting.");
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

// Input code
// Checks bounce values, sidebutton, and keyboard
void bounceSetup() { // Upates input values once per loop after debouncing
  // Check each state
  b1d.update();
  b2d.update();
  b3d.update();
  b4d.update();
  b5d.update();
  b6d.update();
  b7d.update();
  // Write state to array for easy access
  bounce[0] = b1d.read();
  bounce[1] = b2d.read();
  bounce[2] = b3d.read();
  bounce[3] = b4d.read();
  bounce[4] = b5d.read();
  bounce[5] = b6d.read();
  bounce[6] = b7d.read();
}
void sideButton(){
  if(!bounce[numkeys]) { // Change hold value for release
    if (((millis() - sbMillis) < 500)) hold = 1;
    if (((millis() - sbMillis) > 500)) hold = 2;
  }
  if (hold == 2) {
    if (!blink) {
      blinkLED();
      blink = 1;
    }
    for (byte x = 0; x < numkeys; x++) if (!bounce[x]) page = x; // Changes page depending on which key is pressed (1-6)
  }
  if(bounce[numkeys]){
    if (hold == 1) { // Press escape if pressed and released
      Keyboard.press(177);
      delay(8);
      Keyboard.release(177);
    }
    if (hold == 2) EEPROM.write(1, page); // save page value
    hold = 0;
    blink = 0;
    sbMillis = millis();
  }

}
void keyboard(){
  sideButton(); // Run sidebutton code
  if (hold < 2){ // Only run when the side button isn't pressed
    for (int a = 0; a < numkeys; a++) { // checks each key
      if (!pressed[a]) { // This made sense to me at one point
        for (int b = 0; b < 3; b++) if (!bounce[a]) { // For each
          if (mapping[page][a][b] != 0) {
            if (mapping[page][a][b] > 3) Keyboard.press(mapping[page][a][b]);
            else {
              if (mapping[page][a][b] == 1) Mouse.press(MOUSE_LEFT);
              else if (mapping[page][a][b] == 2) Mouse.press(MOUSE_RIGHT);
              else if (mapping[page][a][b] == 3) Mouse.press(MOUSE_MIDDLE);
            }
          }
          pressed[a] = 1; // nonsense
        }
      }
      if (pressed[a]) {
        for (int b = 0; b < 3; b++) if (bounce[a]) {
          if (mapping[page][a][b] != 0) {
              if (mapping[page][a][b] > 3) Keyboard.release(mapping[page][a][b]);
            else {
              if (mapping[page][a][b] == 1) Mouse.release(MOUSE_LEFT);
              else if (mapping[page][a][b] == 2) Mouse.release(MOUSE_RIGHT);
              else if (mapping[page][a][b] == 3) Mouse.release(MOUSE_MIDDLE);
            }
            }
          pressed[a] = 0;
        }
      }
    }
  }
}
