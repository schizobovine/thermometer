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
float avg_temp = 0.0;
int16_t last_temp = 0;
boolean sensor_error = false;

const size_t HISTORY_LENGTH = 4;
int16_t temp_history[HISTORY_LENGTH];
size_t temp_history_pos = 0;
size_t temp_history_max = 0;

// Display
SevSeg display;

const size_t DISP_DIGITS = 3;
const int DISP_ON_TIME_USEC = 100;
const int DISP_OFF_TIME_USEC = 1000;

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

void recordTemp(int16_t temp) {
  //DPRINT(F("temp_history_pos1=")); DPRINTLN(temp_history_pos);
  //DPRINT(F("temp_history_max1=")); DPRINTLN(temp_history_max);
  temp_history[temp_history_pos] = temp;
  temp_history_pos = (temp_history_pos + 1) % HISTORY_LENGTH;
  if (temp_history_max < HISTORY_LENGTH) {
    temp_history_max++;
  }
  //DPRINT(F("temp_history_pos2=")); DPRINTLN(temp_history_pos);
  //DPRINT(F("temp_history_max2=")); DPRINTLN(temp_history_max);
}

float readRollingAvgTemp() {

  int16_t total = 0;
  for (size_t i=0; i<temp_history_max; i++) {
    total += temp_history[i];
  }
  total /= temp_history_max;
  //DPRINT(F("total=")); DPRINTLN(total);

  float temp = total / THERM_DIVIDER;
  //DPRINT(F("temp1=")); DPRINTLN(temp);

  // Peform temp conversion but only compile in code as required
#if   USE_UNIT == UNIT_C
  // Do nothing because the default is C
#elif USE_UNIT == UNIT_F
  temp = temp * 9.0 / 5.0 + 32;
#elif USE_UNIT == UNIT_K
  temp = temp + 273.15;
#else
#error "UNIT NOT DEFINED!"
#endif

  //DPRINT("temp="); DPRINTLN(temp);

  return temp;

}

int16_t readTemp() {
  int16_t temp;
  
  // Return error condition on sensor failure
  if (sensor_error) {
    return 999;
  }

  //therm.shutdown_wake(0);
  temp = therm.readTempRaw();
  //therm.shutdown_wake(1);

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
    //DPRINT(F("last_temp=")); DPRINT(last_temp);
    recordTemp(last_temp);
    avg_temp = readRollingAvgTemp();
    DPRINT(F(" avg_temp=")); DPRINTLN(avg_temp);
  }

  //char dig1, dig2, dig3;
  int8_t dp = 0; // # after . to show
  if (avg_temp <= -10 && avg_temp > -100) {
    // dig1 = '-';
    // dig2 = INT2CHAR(int(avg_temp) / 10);
    // dig3 = INT2CHAR(int(avg_temp) % 10);
    dp = 1;
  } else if (avg_temp < 0 && avg_temp > -10) {
    // dig1 = '-';
    // dig2 = INT2CHAR(int(avg_temp) % 10);
    // dig3 = INT2CHAR(int(avg_temp * 10) % 10);
    dp = 1;
  } else if (avg_temp >= 0 && avg_temp < 10) {
    // dig1 = INT2CHAR(int(avg_temp) % 10);
    // dig2 = INT2CHAR(int(avg_temp * 10) % 10);
    // dig2 = INT2CHAR(int(avg_temp * 100) % 10);
    dp = 1;
  } else if (avg_temp >= 10 && avg_temp < 100) {
    // dig1 = INT2CHAR(int(avg_temp) / 10);
    // dig2 = INT2CHAR(int(avg_temp) % 10);
    // dig3 = INT2CHAR(int(avg_temp * 10) % 10);
    dp = 1;
  } else if (avg_temp >= 100 && avg_temp < 1000) {
    // dig1 = INT2CHAR(int(avg_temp) / 100);
    // dig2 = INT2CHAR(int(avg_temp) / 10);
    // dig3 = INT2CHAR(int(avg_temp) % 10);
    dp = 0;
  } else { // error state
    // dig1 = // dig2 = // dig3 = 'E';
    dp = -1;
  }

  //DPRINT(dig1);
  //DPRINT(dig2);
  //DPRINTLN(dig3);
  if (dp > 0) {
    //display.setNumber(avg_temp, dp);
    display.setNumber(999, 0);
  } else {
    display.setNumber(999, 0);
  }

  display.refreshDisplay(DISP_ON_TIME_USEC);
  delayMicroseconds(DISP_OFF_TIME_USEC);

}
