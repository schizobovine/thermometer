/*
 * thermometer.ino
 *
 * Simple 7-segment display thermometer.
 *
 * Author: Sean Caulfield <sean@yak.net>
 * License: GPL v2.0
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include <Arduino.h>
#include <Adafruit_MCP9808.h>
#include <SevSeg.h>
#include "thermometer.h"

// Temperature sensor
Adafruit_MCP9808 therm = Adafruit_MCP9808();
uint32_t last_check = 0;
float curr_temp = 0.0;

// Display
SevSeg display;

const size_t DISP_DIGITS = 3;
const byte DISP_BRIGHT = 1; // 0-100 for some reason

const byte PINS_DIGITS[DISP_DIGITS] = {
  2, // digit 1 (leftmost)
  5, // digit 2
  6, // digit 3 (rightmost)
};

const byte PINS_SEGMENTS[] = {
  3,  // segment a
  7,  // segment b
  A0, // segment c
  A2, // segment d
  A3, // segment e
  4,  // segment f
  13, // segment g
  A1, // decimal point
};

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

uint32_t time_diff(uint32_t a, uint32_t b) {
  uint32_t retval = 0;

  // Handle wrap-around condition for clock roll-over
  if (a < b) {
    retval = b - a;
  } else {
    retval = (UINT32_MAX - a) + b;
  }

  return retval;
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
  display.begin(COMMON_CATHODE, DISP_DIGITS, PINS_DIGITS, PINS_SEGMENTS);
  display.setBrightness(DISP_BRIGHT);
  display.setNumber(123, 3);

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
  
  // Check if temperature needs updating
  uint32_t now = millis();
  if (time_diff(last_check, now) > THERM_POLL_INTERVAL) {
    last_check = now;
    curr_temp = readTemp();
  }

  /*
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
  */

  //DPRINT(dig1);
  //DPRINT(dig2);
  //DPRINTLN(dig3);

  display.refreshDisplay();
  delayMicroseconds(1000);

}
