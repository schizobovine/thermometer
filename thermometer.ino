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
int8_t display_temp = 0;
boolean sensor_error = false;
boolean metric_units = false;

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
    return 9999;
  }

  float temp;

  //therm.shutdown_wake(0);
  temp = therm.readTempC();
  //therm.shutdown_wake(1);

  DPRINT("tempC="); DPRINT(temp);

  // Convert to Fahrenheit because I suck and can't read Celcius well (yet)
  if (!metric_units) {
    temp = temp * 9.0 / 5.0 + 32;
  }

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
  } else {
    last_temp = readTemp();
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
    display_temp = int(round(last_temp));
  }

  byte digits[4];
  boolean whoops = false;
  if (display_temp <= -10 && display_temp > -100) {
    digits[0] = DASH;
    digits[1] = (-display_temp / 10) % 10;
    digits[2] = (-display_temp) % 10;
  } else if (display_temp < 0 && display_temp > -10) {
    digits[0] = BLANK;
    digits[1] = DASH;
    digits[2] = (-display_temp) % 10;
  } else if (display_temp >= 0 && display_temp < 10) {
    digits[0] = BLANK;
    digits[1] = BLANK;
    digits[2] = (display_temp) % 10;
  } else if (display_temp >= 10 && display_temp < 100) {
    digits[0] = BLANK;
    digits[1] = (display_temp / 10) % 10;
    digits[2] = (display_temp) % 10;
  } else if (display_temp >= 100 && display_temp < 1000) {
    digits[0] = (display_temp / 100) % 10;
    digits[1] = (display_temp / 10) % 10;
    digits[2] = (display_temp) % 10;
  } else {
    whoops = true;
    digits[0] = 0xE;
    digits[1] = 0xE;
    digits[2] = 0xE;
  }

  // either unit or error condition
  if (whoops) {
    digits[3] = 0xE;
  } else if (metric_units) {
    digits[3] = 0xC;
  } else {
    digits[3] = 0xF;
  }

  // Refresh display
  display.setDigits(digits, sizeof(digits));
  display.refreshDisplay(DISP_ON_TIME_USEC);

  // Busy loop so the display isn't burnt out
  delayMicroseconds(DISP_OFF_TIME_USEC);

}
