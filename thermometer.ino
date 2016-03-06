/*
 * thermometer.ino
 *
 * Simple 7-segment display thermometer.
 *
 * Author: Sean Caulfield <sean@yak.net>
 * License: GPL v2.0
 *
 */

//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <math.h>
//#include <avr/io.h>
//#include <avr/pgmspace.h>

#include <Arduino.h>
#include <Adafruit_MCP9808.h>

// Temperature sensor
Adafruit_MCP9808 therm = Adafruit_MCP9808();

// What unit to use for display?
#define UNIT_C 0
#define UNIT_F 1
#define UNIT_K 2
#define USE_UNIT UNIT_F

// Only really used for serial connection
#if   USE_UNIT == UNIT_C
#define UNIT_SYMBOL F("C")
#elif USE_UNIT == UNIT_F
#define UNIT_SYMBOL F("F")
#elif USE_UNIT == UNIT_K
#define UNIT_SYMBOL F("K")
#else
#error "USE_UNIT not defined properly!"
#endif

#define INT2CHAR(x) ((char)((x) + 0x30))

// Seven segment pin mappings
#define PIN_SEG_A 3
#define PIN_SEG_B 7
#define PIN_SEG_C A0
#define PIN_SEG_D A2
#define PIN_SEG_E A3
#define PIN_SEG_F 4
#define PIN_SEG_G 13
#define PIN_SEG_P A1
#define PIN_DIG_1 2
#define PIN_DIG_2 5
#define PIN_DIG_3 6

////////////////////////////////////////////////////////////////////////
// DEBUGGING
////////////////////////////////////////////////////////////////////////

#define SERIAL_DEBUG 1
#define SERIAL_BAUD 9600

#ifdef SERIAL_DEBUG
#define DPRINT(...) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINTLN(...)
#endif

////////////////////////////////////////////////////////////////////////
// HALPING
////////////////////////////////////////////////////////////////////////

float readTemp() {
  float temp;
  
  //thermo.shutdown_wake(0);
  temp = therm.readTempC();
  //thermo.shutdown_wake(1);

  // Peform temp conversion but only compile in code as required
#if   USE_UNIT == USE_C
  // Do nothing because the default is C
#elif USE_UNIT == USE_F
  temp = temp * 9.0 / 5.0 + 32;
#elif USE_UNIT == USE_K
  temp = temp + 273.15;
#endif

  DPRINT(temp, 2);
  DPRINT(F(" "));
  DPRINTLN(UNIT_SYMBOL);

  return temp;
}

////////////////////////////////////////////////////////////////////////
// SETUP
////////////////////////////////////////////////////////////////////////

void setup() {

#if SERIAL_DEBUG
  delay(200);
  Serial.begin(SERIAL_BAUD);
#endif

  // Initialize display

  // Find and prep temp sensor
  if (!therm.begin()) {
    DPRINTLN(F("Couldn't find sensor!"));
    // TODO set display to fail mode
  }

}

////////////////////////////////////////////////////////////////////////
// MAIN LOOP
////////////////////////////////////////////////////////////////////////

void loop() {
  
  float temp = readTemp();
  char dig1, dig2, dig3;
  uint8_t dp = 0;

  if (temp <= -10 && temp > -100) {
    dig1 = '-';
    dig2 = INT2CHAR(int(temp) / 10);
    dig3 = INT2CHAR(int(temp) % 10);
    dp = 0;
  } else if (temp < 0 && temp > -10) {
    dig1 = '-';
    dig2 = INT2CHAR(int(temp) % 10);
    dig3 = INT2CHAR(int(temp * 10) % 10);
    dp = 2;
  } else if (temp >= 0 && temp < 10) {
    dig1 = INT2CHAR(int(temp) % 10);
    dig2 = INT2CHAR(int(temp * 10) % 10);
    dig2 = INT2CHAR(int(temp * 100) % 10);
    dp = 1;
  } else if (temp >= 10 && temp < 100) {
    dig1 = INT2CHAR(int(temp) / 10);
    dig2 = INT2CHAR(int(temp) % 10);
    dig3 = INT2CHAR(int(temp * 10) % 10);
    dp = 2;
  } else if (temp >= 100 && temp < 1000) {
    dig1 = INT2CHAR(int(temp) / 100);
    dig2 = INT2CHAR(int(temp) / 10);
    dig3 = INT2CHAR(int(temp) % 10);
    dp = 0;
  } else {
    dig1 = dig2 = dig3 = 'E';
  }

  DPRINT(dig1);
  DPRINT(dig2);
  DPRINTLN(dig3);

  delay(1000);

}
