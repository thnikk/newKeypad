#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// Uncomment line below for debug code
// #define serialDebug

byte numkeys = 8; // Number of keys
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(11, 17, NEO_GRBW + NEO_KHZ800);
byte rgb[11][3]; // RGB color array

// Inputs (to buttons)
byte input[8] = { 0, 1, 2, 3, 4, 5, 6, 7 }; // left, up, right, down, square, triangle, circle, cross
byte lr[2] = { 8, 9 };

byte output[2][8] = { // Outputs mapped normally and in FT mode (should automate)
  // triangle 18, square 11, circle 12, cross 13, right 14, left 15, down 16, up 17
  { 21, 23, 20, 22, 16, 15, 18, 19 },
  { 15, 23, 16, 21, 19, 22, 18, 20 }  // triangle, up, square, left, cross, down, circle, right
};

byte sweepMap[2][7] = { // 1=l2, 2=up, 3=left, 4=down, 5=right, 6=square, 7=cross, 8=circle, 9=triangle, 10=r2
  { 1, 2, 5, 255, 6, 9, 10 },
  { 3, 4, 255, 255, 255, 7, 8 }
};

// Color definitions
byte red = 0;
byte orange = 28;
byte yellow = 40;
byte green = 85;
byte blue = 170;
byte purple = 200;
byte pink = 230;

byte colors[2][11] ={ // Color array for modes
  { pink, yellow, green, red, blue, pink, red, blue, pink, green, yellow },
  { purple, yellow, blue, red, red, blue, pink, pink, green, green, yellow }
};

byte pressed[8];

bool version = 0;
byte mode = 0;
byte b = 100; // Brightness
byte modeButton = 12;

unsigned long previousMillis = 0;
bool test = 0;

#ifdef serialDebug
  unsigned long pollMillis = 0;
  long count = 0;
#endif

void setup(){
  Serial.begin(9600);
  if(EEPROM.read(0) != version){
    EEPROM.write(0, version);
    EEPROM.write(1, 0);
  }
  mode = EEPROM.read(1);
  // set inputs to pullup
  for (byte x=0; x<numkeys; x++) pinMode(input[x], INPUT_PULLUP);
  for (byte x=0; x<2; x++) pinMode(lr[x], INPUT_PULLUP);
  // set outputs to output (only first because they do the same thing)
  for (byte x=0; x<numkeys; x++) pinMode(output[0][x], OUTPUT);
  // pinMode(13, LOW); // Turn off activity LED
  pinMode(modeButton, INPUT_PULLUP); // Set mode pin to pullup
  pixels.begin();
}

byte colorBrightness[8];
byte lrBrightness[2];

void loop(){

  // Convert inputs to outputs
  for (byte x=0; x<numkeys; x++) {
    if (digitalRead(input[x]) == 0) {
      pressed[x] = 1;
      digitalWrite(output[mode][x], LOW);
    }
    if (!digitalRead(input[x]) == 0) {
      pressed[x] = 0;
      digitalWrite(output[mode][x], HIGH);
    }
  }


  // Mode button
  if (digitalRead(modeButton)) {
    if (test == 1) {
      mode++;
      if (mode > 1) mode = 0;
      EEPROM.write(1, mode);
      test = 0;
    }
  }
  if (!digitalRead(modeButton)) {
    if (test == 0) test = 1;
  }

  // LEDs
  if ((millis() - previousMillis) > 1) {
    colorPress();
    colorSweep(); // writes over original colors
    pixels.show();
    previousMillis = millis();
  }

  #ifdef serialDebug
    for(byte x=0; x<2; x++) {
      // Serial.print(digitalRead(lr[x]));
      Serial.print(lrBrightness[x]);
      Serial.print(", ");
    }
    Serial.println();
    // Poll check
    count++; // Add to counter once per loop
    if ((millis() - pollMillis) > 1000) {
      Serial.println(count/1000); // print counter / 1000 for loops per ms (rounded down)
      count = 0; // clear counter
      pollMillis = millis();
    }
  #endif
}

// byte input[8] = { 0, 1, 2, 3, 4, 5, 6, 7 }; // left, up, right, down, square, triangle, circle, cross
// green, red, blue, pink, red, blue, pink, green,
// Inputs to colors
byte inToColors[8] = { 5, 6, 7, 4, 2, 3, 0, 1};

void colorPress(){
  for (byte x=0; x < 11; x++){
    wheel(colors[mode][x], x);
    setLED(x);
  }
  for (byte z=0; z<8; z++) {
    if (!digitalRead(inToColors[z]) && colorBrightness[z] > 0) colorBrightness[z] = colorBrightness[z] - 3;// ledOff(z+2);
    if (digitalRead(inToColors[z])) colorBrightness[z] = 255;
  }
  for (byte x=0; x<2; x++){
    if (!digitalRead(lr[x]) && lrBrightness[x] > 0) lrBrightness[x] = lrBrightness[x] - 3;// ledOff(z+2);
    if (digitalRead(lr[x])) lrBrightness[x] = 255;
  }
}

void wheel(byte shortColor, byte key) { // Set RGB color with byte
  if (shortColor >= 0 && shortColor < 85) { rgb[key][0] = (shortColor * -3) +255; rgb[key][1] = shortColor * 3; rgb[key][2] = 0; }
  else if (shortColor >= 85 && shortColor < 170) { rgb[key][0] = 0; rgb[key][1] = ((shortColor - 85) * -3) +255; rgb[key][2] = (shortColor - 85) * 3; }
  else { rgb[key][0] = (shortColor - 170) * 3; rgb[key][1] = 0; rgb[key][2] = ((shortColor - 170) * -3) +255; }
}

byte sweepB[11] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

void setLED(byte led) {
  byte rgbBuff[3];
  for(byte x=0; x<3; x++) {
    rgbBuff[x] = rgb[led][x];
    rgbBuff[x] = b*rgbBuff[x]/255; // Brightness multiplier
    if (led == 1) rgbBuff[x] = lrBrightness[1]*rgbBuff[x]/255;
    if (led >= 2 && led < 10) rgbBuff[x] = colorBrightness[led-2]*rgbBuff[x]/255; // colorPress multiplier
    if (led == 10) rgbBuff[x] = lrBrightness[0]*rgbBuff[x]/255;
    if (led > 0) rgbBuff[x] = sweepB[led]*rgbBuff[x]/255;
  }
  pixels.setPixelColor(led, pixels.Color(rgbBuff[0], rgbBuff[1], rgbBuff[2], 0));
}

void ledOff(byte led) {
  pixels.setPixelColor(led, pixels.Color(0,0,0,0));
}

byte lCount = 0;
byte rCount = 0;

unsigned long sMillis;
void colorSweep() {
  if (millis() - sMillis > 0) { // limit effects to run once per ms

    if (!digitalRead(lr[1])){
      for (byte x=0; x<1; x++){
        for (byte y=0; y<2; y++){
          byte led = sweepMap[y][x];
          sweepB[led] = sweepB[led]-=1;
        }
      }
    }
    if (digitalRead(lr[1])){
      for (byte x=0; x<1; x++){
        for (byte y=0; y<2; y++){
          byte led = sweepMap[y][x];
          sweepB[led] = 255;
        }
      }
    }

  sMillis = millis();
  }
}

// byte sweepCol = 0;
// unsigned long sml; // sweep millis timer
// unsigned long smr;
// unsigned long speedMult = 30;
// byte oldCol = 255;
// byte sweepB[7]; // sweep brightness per column
// void colorSweep() {
//  if (!digitalRead(lr[0])) for(byte x=0;x<8;x++) if (millis() - sml > (speedMult * x) && millis() - sml < (speedMult * x) + speedMult) sweepCol = x;
//  if (millis()-sml > (speedMult*8)) sml = millis();
//  if (!digitalRead(lr[1])) for(byte x=0;x<8;x++) if (millis() - smr > (speedMult * x) && millis() - smr < (speedMult * x) + speedMult) sweepCol = (x - 6) * -1;
//  if (millis()-smr > (speedMult*8)) smr = millis();
//  if (digitalRead(lr[0])) sml = millis();
//  if (digitalRead(lr[1])) smr = millis();
//  if (digitalRead(lr[0]) && digitalRead(lr[1])) sweepCol = -1;
//  // Print sweepB values to serial monitor
//  Serial.print("[ ");
//  for (byte x=0;x<7;x++) {
//    Serial.print(sweepB[x]);
//    if(x!=6)Serial.print(", ");
//  }
//  Serial.println(" ]");
//  // sets leds
//  for(byte x=0; x<2; x++) if (sweepMap[x][sweepCol] < 11 && sweepCol >=0 && sweepCol < 7) pixels.setPixelColor(sweepMap[x][sweepCol], pixels.Color(100,100,0,0));
//}
