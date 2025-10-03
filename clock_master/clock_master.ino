//==================================================================================
// LIBRARIES
//==================================================================================
#include <DS3231.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

//==================================================================================
// Pins
//==================================================================================
const byte PIN_2 = 2, PIN_3 = 3, PIN_4 = 4;

//==================================================================================
// MSF BIT ARRAY + PARITY
//==================================================================================
volatile int bits[59][2];
int count = 0;
short bad_pulse_count = 0;
bool parA_valid = false, parB_valid = false, parC_valid = false, parD_valid = false;
short parA = 0, parB = 0, parC = 0, parD = 0;

//==================================================================================
// CONNECTION TIMERS
//==================================================================================
unsigned long start_time;
unsigned long end_time;
unsigned long pulse_duration;
unsigned long pulse_off_duration;
unsigned long prev_pulse_off_duration;
unsigned long prev_millis_time = 0;
const int interval_time = 1000; 

//==================================================================================
// TIME & INITIALISATION
//==================================================================================
int year, month, day, dow, hour, min;

DS3231 rtc(SDA, SCL);
SoftwareSerial gpsSerial(PIN_2, PIN_4);
TinyGPSPlus gps;

//==================================================================================
// SETUP
//==================================================================================
void setup() {
  Serial.begin(9600);
  rtc.begin();
  gpsSerial.begin(9600);
  pinMode(PIN_2, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_2), read_pulse, CHANGE);
}


//==================================================================================
// MAIN LOOP
//==================================================================================
void loop() {
  unsigned long current_millis = millis();
  while (gpsSerial.available() > 0){
    if (gps.encode(gpsSerial.read()))
      rtc.setTime(gps.time.hour() + 1, gps.time.minute(), gps.time.second());
      rtc.setDate(gps.date.day(), gps.date.month(), gps.date.year());
      delay(200);
  }
}

//==================================================================================
// HELPER FUNCTIONS
//==================================================================================

void calc_time(){
  // --- MSF TIME CALCULATION ---
  int j = 0;
  int sum = 0;
  
  // Time bit weights
  const int wyr[8] = {80, 40, 20, 10, 8, 4, 2, 1};
  const int wmon[5] = {10, 8, 4, 2, 1};
  const int wday[6] = {20, 10, 8, 4, 2, 1};
  const int wdow[3] = {4, 2, 1};
  const int whour[6] = {20, 10, 8, 4, 2, 1};
  const int wmin[7] = {40, 20, 10, 8, 4, 2, 1};

 // Iterate through complete bit array
 // The corresponding if statement sums the weighted bits then is saved to the time variable in the next statement.
 for(int i=16; i<58; i++){
     // Bits (17-24) code the year
    if(i < 24){
      if(bits[i][0] == 1){
        parA += 1;
      }
      sum += bits[i][0]*wyr[j]; 
    }

    // Bits (25-29) code the month (1-12)
    else if(i < 29){
      if(i == 24){
        year = sum;
        sum = 0;
        j = 0;
      }
      if(bits[i][0] == 1){
        parB += 1;
      }
      sum += bits[i][0]*wmon[j];
    }

    // Bits (30-35) code the day of the month (1-31)
    else if(i < 35){
      if(i == 29){
        month = sum;
        sum = 0;
        j = 0;
      }
      if(bits[i][0] == 1){
        parB += 1;
      }
      sum += bits[i][0]*wday[j]; 
    }
    
    // Bits (39-44) code the day of the week (0-6, 0 being Sunday)
    else if(i < 38){
      if(i == 35){
        day = sum;
        sum = 0;
        j = 0;
      }
      if(bits[i][0] == 1){
        parC += 1;
      }
      sum += bits[i][0]*wdow[j];
    }

    // Bits (39-44) code the hour
    else if(i < 44){
      if(i == 38){
        dow = sum;
        sum = 0;
        j = 0;
      }
      if(bits[i][0] == 1){
        parD += 1;
      }
      sum += bits[i][0]*whour[j];  
    }
    
    // Bits (45-51) code the minutes
    else if(i < 51){
      if(i == 44){
        hour = sum;
        sum = 0;
        j = 0;
      }
      if(bits[i][0] == 1){
        parD += 1;
      }
      sum += bits[i][0]*wmin[j]; 
    }

    else if(i == 51){
      min = sum;
      sum = 0;
      j = 0;
    }
    j += 1;
  }

  // Serial.print(day);
  // Serial.print("/");
  // Serial.print(month);
  // Serial.print("/");
  // Serial.print(2000 + year);
  // Serial.print(" ");
  // Serial.print(hour);
  // Serial.print(":");
  // Serial.println(min);

  // Check parity
  if((parA + bits[53][1]) % 2 == 1){
    Serial.println(("YEAR IS VALID"));
    parA_valid = true;
  }
  if((parB + bits[54][1]) % 2 == 1){
    Serial.println(("MONTH IS VALID"));
    parB_valid = true;
  }
  if((parC + bits[55][1]) % 2 == 1){
    Serial.println(("DAY OF THE WEEK IS VALID"));
    parC_valid = true;
  }
  if((parD + bits[56][1]) % 2 == 1){
    Serial.println(("HOUR + MINUTE IS VALID"));
    parD_valid = true;
  }
  if(parA_valid == true && parB_valid == true && parC_valid == true && parD_valid == true && bad_pulse_count == 1){
    rtc.setTime(hour, min, 0);
    rtc.setDate(day, month, 2000 + year);
    if(dow == 0){
      dow = 7;
    }
    rtc.setDOW(dow);
  }

  parA = 0, parB = 0, parC = 0, parD = 0;
  parA_valid = false, parB_valid = false, parC_valid = false, parD_valid = false;
}

// Read pulse duration
void read_pulse(){
  if (digitalRead(PIN_2) == HIGH){
    pulse_off_duration = micros() - (start_time + pulse_duration);
    Serial.print(", OFF: ");
    Serial.print(pulse_off_duration/1000);
    Serial.println();
    start_time = micros();
    check_bits();
  }
  else{
    end_time = micros();
    pulse_duration = end_time - start_time;
    prev_pulse_off_duration = pulse_off_duration;
    // Serial.print("PREV OFF: ");
    // Serial.print(prev_pulse_off_duration/1000);
    Serial.print(", ON: ");
    Serial.print(pulse_duration/1000);
  }
}

// Defines the type of signal to a 2 digit bit array
void check_bits(){
  if(prev_pulse_off_duration >= 800000 && prev_pulse_off_duration <= 1000000 && pulse_duration >= 400000 && pulse_duration <= 600000 && pulse_off_duration >= 400000 && pulse_off_duration <= 600000){
    Serial.println("NEW MINUTE STARTED");
    Serial.println(bad_pulse_count);
    calc_time();
    bad_pulse_count = 0;
    count = 0;
    for(int i=0; i<58; i++){
      Serial.print("(");  
      Serial.print(bits[i][0]);
      Serial.print(",");
      Serial.print(bits[i][1]);
      Serial.print("), ");
      }
  }
  else if(pulse_duration >= 250000 && pulse_duration <= 350000 && pulse_off_duration >= 600000 && pulse_off_duration <= 800000 ){
    count += 1;
    bits[count][0] = 1, bits[count][1] = 1;
  }
  else if(pulse_duration >=160000 && pulse_duration <= 250000 && pulse_off_duration >= 700000 && pulse_off_duration <= 900000 ){
    count += 1;
    bits[count][0] = 1, bits[count][1] = 0; 
  }
  else if(pulse_duration >=50000 && pulse_duration <= 160000 && prev_pulse_off_duration >= 50000 && prev_pulse_off_duration <= 150000 && pulse_off_duration >= 600000 && pulse_off_duration <= 800000){
    count += 1;
    bits[count][0] = 0, bits[count][1] = 1;
  }
  else if(pulse_duration >=50000 && pulse_duration <= 160000 && pulse_off_duration >= 800000 && pulse_off_duration <= 1000000){
    count += 1;
    bits[count][0] = 0, bits[count][1] = 0;
  }
  else{
    bad_pulse_count += 1;
  }
}
