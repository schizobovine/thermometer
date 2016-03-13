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
float last_temp = 0.0;
boolean sensor_error = false;

// Display
SevSeg display;

const size_t DISP_DIGITS = 4;
const int DISP_ON_TIME_USEC = 100;
const int DISP_OFF_TIME_USEC = 1000;

const byte PINS_DIGITS[DISP_DIGITS] = {
  2, // digit 1 (leftmost)
  5, // digit 2
  6, // digit 3
  12, // digit 4 (rightmost)
};

const byte PINS_SEGMENTS[] = {
  3,  // segment a
  7,  // segment b
  A0, // segment c
  A2, // segment d
  A3, // segment e
  4,  // segment f
  13, // segment g
  A1, // decimal point <-- doesn't actually work with HS420561K-C30
};

////////////////////////////////////////////////////////////////////////
// HALPING
////////////////////////////////////////////////////////////////////////

float readTemp() {
  
  // Return error condition on sensor failure
  if (sensor_error) {
    return 999;
  }

  float temp;

  //therm.shutdown_wake(0);
  temp = therm.readTempC();
  //therm.shutdown_wake(1);


  // Convert to Fahrenheit because I suck and can't read C well (yet)
  temp = temp * 9.0 / 5.0 + 32;

  DPRINT( "tempC="); DPRINT(temp);
  DPRINT(" tempF="); DPRINTLN(temp);

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

  // Setup display
  display.begin(COMMON_CATHODE, DISP_DIGITS, PINS_DIGITS, PINS_SEGMENTS);

  // Find and prep temp sensor
  if (!therm.begin()) {
    DPRINTLN(F("Couldn't find sensor!"));
    sensor_error = true;
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
    last_temp = readTemp();
  }

  int8_t dp = 0; // # after . to show
  if (last_temp <= -10 && last_temp > -100) {
    dp = 1;
  } else if (last_temp < 0 && last_temp > -10) {
    dp = 1;
  } else if (last_temp >= 0 && last_temp < 10) {
    dp = 1;
  } else if (last_temp >= 10 && last_temp < 100) {
    dp = 1;
  } else if (last_temp >= 100 && last_temp < 1000) {
    dp = 0;
  } else { // error state
    dp = -1;
  }

  if (dp > 0) {
    display.setNumber(last_temp, dp);
    //display.setNumber(999, 0);
  } else {
    display.setNumber(999, 0);
  }

  display.refreshDisplay(DISP_ON_TIME_USEC);
  delayMicroseconds(DISP_OFF_TIME_USEC);

}
