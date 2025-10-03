/*
- Clock Display Arduino Program
- By Jakub M
- 03/10/2025
- Version: 1.
*/ 

//==================================================================================
// LIBRARIES
//==================================================================================
#include <FastLED.h>
#include <DS3231.h>
#include <BH1750.h>
#include <EEPROM.h>

//==================================================================================
// LED DISPLAY CONFIGURATION
//==================================================================================
const byte NUM_LEDS = 39;
const byte PIN_4 = 4, PIN_5 = 5, PIN_6 = 6, PIN_7 = 7, PIN_8 = 8, PIN_9 = 9, PIN_10 = 10;
const byte PIN_DT = A0, PIN_CLK = 2, PIN_BTN = 12, PIN_DT2 = 11, PIN_CLK2 = 3, PIN_BTN2 = A1;

const byte DIGIT_POSITIONS[6] = {0,6,14,20,28,34};
CHSV colour = CHSV(0, 255, 255);

//==================================================================================
// CONNECTION TIMERS
//==================================================================================
const int INTERVAL_UPDATE_TIME = 500; 
const int INTERVAL_LIGHT_TIME = 30000;
unsigned long prev_millis_time = 0;
unsigned long prev_millis_time2 = 0;

//==================================================================================
// ENCODER/ BUTTON STATES
//==================================================================================
unsigned long last_button_press = 0;
unsigned long last_button_press2 = 0;
int current_state_CLK, current_state_CLK2;
int last_state_CLK, last_state_CLK2;
bool bright_manual = true;
float last_lux;

//==================================================================================
// LED ARRAYS + init
//==================================================================================
CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];
CRGB leds5[NUM_LEDS];
CRGB leds6[NUM_LEDS];
CRGB leds7[NUM_LEDS];

DS3231 rtc(SDA, SCL);
BH1750 lightmeter;


//==================================================================================
// SETUP
//==================================================================================
void setup() {
  Serial.begin(9600);

  // Init
  rtc.begin();
  lightmeter.begin();

  //Pins
  pinMode(pin_CLK, INPUT);
  pinMode(pin_DT, INPUT);
  pinMode(pin_btn, INPUT_PULLUP);
  pinMode(pin_CLK2, INPUT);
  pinMode(pin_DT2, INPUT);
  pinMode(pin_btn2, INPUT_PULLUP);

  // LED arrays
  FastLED.addLeds<WS2812, PIN_10, GRB>(leds1, NUM_LEDS);
  FastLED.addLeds<WS2812, PIN_9, GRB>(leds2, NUM_LEDS);
  FastLED.addLeds<WS2812, PIN_8, GRB>(leds3, NUM_LEDS);
  FastLED.addLeds<WS2812, PIN_7, GRB>(leds4, NUM_LEDS);
  FastLED.addLeds<WS2812, PIN_6, GRB>(leds5, NUM_LEDS);
  FastLED.addLeds<WS2812, PIN_5, GRB>(leds6, NUM_LEDS);
  FastLED.addLeds<WS2812, PIN_4, GRB>(leds7, NUM_LEDS);

  // Save settings on boot
  //FastLED.setBrightness(15);  
  bright_manual = EEPROM.read(2);
  FastLED.setBrightness(EEPROM.read(1));
  colour.hue = EEPROM.read(0);
  last_lux = FastLED.getBrightness();
  FastLED.clear();

  // Read the initial state of CLK
  last_state_CLK = digitalRead(pin_CLK);

  // Call updateEncoder() when a change is seen on CLK pin
  attachInterrupt(digitalPinToInterrupt(pin_CLK), updateEncoderBright, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_CLK2), updateEncoderHue, CHANGE);
}

//==================================================================================
// MAIN LOOP
//==================================================================================
void loop() {
  unsigned long current_millis = millis();

  int btnState = digitalRead(pin_btn);
  int btnState2 = digitalRead(pin_btn2);

  //If we detect LOW signal, button is pressed
  if (btnState == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - last_button_press > 50) {
      Serial.println("Button pressed!");
      FastLED.clear();
      leds5[12] = colour, leds5[26] = colour;
      leds6[12] = colour, leds6[26] = colour;
      byte date = rtc.getTime().date;
      byte mon = rtc.getTime().mon;
      int year = rtc.getTime().year;
      // date
      display(DIGIT_POSITIONS[0], floor(date/10));
      display(DIGIT_POSITIONS[1], date % 10);
      //mon
      display(DIGIT_POSITIONS[2], floor(mon/10));
      display(DIGIT_POSITIONS[3], mon % 10);
      //year
      display(DIGIT_POSITIONS[4], floor(year/1000));
      display(DIGIT_POSITIONS[5], year % 10);

      FastLED.show();
      delay(3000);
    }

    // Remember last button press event
    last_button_press = millis();
  }

    if (btnState2 == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - last_button_press2 > 50) {
        if (bright_manual == true){
          bright_manual = false;
          EEPROM.write(2, bright_manual);
        }
        else{
          bright_manual = true;
          FastLED.setBrightness(EEPROM.read(1));
          EEPROM.write(2, bright_manual);
        }
    }
    // Remember last button press event
    last_button_press2 = millis();
  }

  if (current_millis - prev_millis_time2 >= INTERVAL_LIGHT_TIME) {
    if (bright_manual == false) {
      float lux = lightmeter.readLightLevel();

      if(last_lux > lux){
        uint8_t bright = FastLED.getBrightness();
        if((bright - 3) < 0){
          bright = 1;
          FastLED.setBrightness(1);
        }
        else{
        FastLED.setBrightness(bright - 3);
        }
      }
      else if(last_lux < lux){
          uint8_t bright = FastLED.getBrightness();
          if((bright + 3) > 35){
            bright = 35;
          }
          FastLED.setBrightness(bright + 3);
      }
      last_lux = lux;
    }
    prev_millis_time2 = current_millis;
  }

  if (current_millis - prev_millis_time >= INTERVAL_UPDATE_TIME) {
    FastLED.clear();
    leds3[12] = colour, leds3[26] = colour;
    leds5[12] = colour, leds5[26] = colour;
    byte hour = rtc.getTime().hour;
    byte min = rtc.getTime().min;
    byte sec = rtc.getTime().sec;
    // hours
    display(DIGIT_POSITIONS[0], floor(hour/10));
    display(DIGIT_POSITIONS[1], hour % 10);
    //minutes
    display(DIGIT_POSITIONS[2], floor(min/10));
    display(DIGIT_POSITIONS[3], min % 10);
    //seconds
    display(DIGIT_POSITIONS[4], floor(sec/10));
    display(DIGIT_POSITIONS[5], sec % 10);
    
    FastLED.show();
    prev_millis_time = current_millis;
  }
} 

// =========================================================
// HELPER FUNCTIONS
// =========================================================
void updateEncoderBright() {
  if (bright_manual == true){
  current_state_CLK = digitalRead(pin_CLK);

  if (current_state_CLK != last_state_CLK && current_state_CLK == 1) {
    if (digitalRead(pin_DT) != current_state_CLK) {
        bright_plus();
    } else {
        bright_minus();
    }
  }
  last_state_CLK = current_state_CLK;
  }
}


void updateEncoderHue() {
  // Read the current state of CLK
  current_state_CLK2 = digitalRead(pin_CLK2);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (current_state_CLK2 != last_state_CLK2 && current_state_CLK2 == 1) {
    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(pin_DT2) != current_state_CLK2) {
        hue_plus();
    } else {
      // Encoder is rotating CW so increment
        hue_minus();
    }
  }
  // Remember last CLK state
  last_state_CLK2 = current_state_CLK2;
}


void hue_plus(){
  colour.hue += 10;
  EEPROM.write(0, colour.hue);
}

void hue_minus(){
  colour.hue -= 10;
  EEPROM.write(0, colour.hue);
}

void bright_plus(){
  uint8_t bright = FastLED.getBrightness();
  Serial.println(bright);
  if((bright + 2) > 35){
    bright = 35;
  }
  FastLED.setBrightness(bright + 2);
  EEPROM.write(1, bright + 2);
}

void bright_minus(){
  uint8_t bright = FastLED.getBrightness();
  Serial.println(bright);
  if((bright - 2) < 0){
    bright = 1;
    FastLED.setBrightness(1);
    EEPROM.write(1, 1);
  }
  else{
  FastLED.setBrightness(bright - 2);
  EEPROM.write(1, bright - 2);
  }
}

// =========================================================
// DISPLAY FUNCTION
// =========================================================
void display(byte position, byte value){
  if (value == 1){
    leds1[position + 2] = colour;
    leds2[position + 1] = colour, leds2[position + 2] = colour;
    leds3[position + 2] = colour;
    leds4[position + 2] = colour;
    leds5[position + 2] = colour;
    leds6[position + 2] = colour;
    leds7[position + 0] = colour, leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour, leds7[position + 4] = colour;
  }

  else if (value == 2){
    leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour;
    leds2[position + 0] = colour, leds2[position + 4] = colour;
    leds3[position + 4] = colour;
    leds4[position + 2] = colour, leds4[position + 3] = colour;
    leds5[position + 1] = colour;
    leds6[position + 0] = colour, leds6[position + 4] = colour;
    leds7[position + 0] = colour, leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour, leds7[position + 4] = colour;
  }

  else if (value == 3){
    leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour;
    leds2[position + 0] = colour, leds2[position + 4] = colour;
    leds3[position + 4] = colour;
    leds4[position + 1] = colour, leds4[position + 2] = colour, leds4[position + 3] = colour;
    leds5[position + 4] = colour;
    leds6[position + 0] = colour, leds6[position + 4] = colour;
    leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour;
  }

  else if (value == 4){
    leds1[position + 3] = colour, leds1[position + 4] = colour;
    leds2[position + 2] = colour, leds2[position + 4] = colour;
    leds3[position + 1] = colour, leds3[position + 4] = colour;
    leds4[position + 0] = colour, leds4[position + 4] = colour;
    leds5[position + 0] = colour, leds5[position + 1] = colour, leds5[position + 2] = colour, leds5[position + 3] = colour, leds5[position + 4] = colour;
    leds6[position + 4] = colour;
    leds7[position + 4] = colour;
  }

  else if (value == 5){
    leds1[position + 0] = colour, leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour, leds1[position + 4] = colour;
    leds2[position + 0] = colour;
    leds3[position + 0] = colour, leds3[position + 1] = colour, leds3[position + 2] = colour, leds3[position + 3] = colour;
    leds4[position + 4] = colour;
    leds5[position + 4] = colour;
    leds6[position + 0] = colour, leds6[position + 4] = colour;
    leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour;
  }

  else if (value == 6){
    leds1[position + 2] = colour, leds1[position + 3] = colour;
    leds2[position + 1] = colour;
    leds3[position + 0] = colour;
    leds4[position + 0] = colour, leds4[position + 1] = colour, leds4[position + 2] = colour, leds4[position + 3] = colour;
    leds5[position + 0] = colour, leds5[position + 4] = colour;
    leds6[position + 0] = colour, leds6[position + 4] = colour;
    leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour;
  }

  else if (value == 7){
    leds1[position + 0] = colour, leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour, leds1[position + 4] = colour;
    leds2[position + 0] = colour, leds2[position + 4] = colour;
    leds3[position + 4] = colour;
    leds4[position + 3] = colour;
    leds5[position + 2] = colour;
    leds6[position + 2] = colour;
    leds7[position + 2] = colour;
  }

  else if (value == 8){
    leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour;
    leds2[position + 0] = colour, leds2[position + 4] = colour;
    leds3[position + 0] = colour, leds3[position + 4] = colour;
    leds4[position + 1] = colour, leds4[position + 2] = colour, leds4[position + 3] = colour;
    leds5[position + 0] = colour, leds5[position + 4] = colour;
    leds6[position + 0] = colour, leds6[position + 4] = colour;
    leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour;
  }

  else if (value == 9){
    leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour;
    leds2[position + 0] = colour, leds2[position + 4] = colour;
    leds3[position + 0] = colour, leds3[position + 4] = colour;
    leds4[position + 1] = colour, leds4[position + 2] = colour, leds4[position + 3] = colour, leds4[position + 4] = colour;
    leds5[position + 4] = colour;
    leds6[position + 3] = colour;
    leds7[position + 1] = colour, leds7[position + 2] = colour;
  }

  else if (value == 0){
    leds1[position + 1] = colour, leds1[position + 2] = colour, leds1[position + 3] = colour;
    leds2[position + 0] = colour, leds2[position + 4] = colour;
    leds3[position + 0] = colour, leds3[position + 3] = colour, leds3[position + 4] = colour;
    leds4[position + 0] = colour, leds4[position + 2] = colour, leds4[position + 4] = colour;
    leds5[position + 0] = colour, leds5[position + 1] = colour, leds5[position + 4] = colour;
    leds6[position + 0] = colour, leds6[position + 4] = colour;
    leds7[position + 1] = colour, leds7[position + 2] = colour, leds7[position + 3] = colour;
  }
}

