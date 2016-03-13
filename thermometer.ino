/*
 * thermometer.ino
 *
 * Simple 7-segment display thermometer.
 *
 * Author: Sean Caulfield <sean@yak.net>
 * License: GPL v2.0
 *
 */

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include <Arduino.h>
#include <Adafruit_MCP9808.h>
#include <Bounce2.h>
#include <SevSeg.h>

#include "thermometer.h"

// Temperature sensor variables/consts
Adafruit_MCP9808 therm = Adafruit_MCP9808();
uint32_t last_check = 0;
uint32_t THERM_POLL_USEC = 1000000; // microseconds
int curr_temp = 0;
int last_temp = 0;
boolean sensor_error = false;
boolean metric_units = false;

// Display variables/consts
SevSeg display;

const size_t DISP_DIGITS = 4;
const int DISP_ON_TIME_USEC = 1000;
const int DISP_OFF_TIME_USEC = 10000;

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

//uint32_t usec_acc = 0;
//uint16_t counter = 0;
//uint32_t temp_acc = 0;
//uint16_t temp_counter = 0;
//const uint16_t LOOPS = 1000;
//const uint16_t TEMP_LOOPS = 10;

// Button variables/consts
Bounce butt;
const byte BUTT_PIN = 9;
const uint16_t BUTT_DEBOUNCE_MS = 50;

////////////////////////////////////////////////////////////////////////
// HALPING
////////////////////////////////////////////////////////////////////////

int readTemp() {
  
  // Return error condition on sensor failure
  if (sensor_error) {
    return 9999;
  }

  float temp;

  therm.shutdown_wake(0);
  temp = therm.readTempC();
  therm.shutdown_wake(1);

  //DPRINT("tempC="); DPRINT(temp);

  // Convert to Fahrenheit because I suck and can't read Celcius well (yet)
  if (!metric_units) {
    temp = temp * 9.0 / 5.0 + 32;
  }

  //DPRINT(" tempF="); DPRINTLN(temp);

  return int(temp);

}

uint32_t time_diff(const uint32_t a, const uint32_t b) {
  uint32_t retval = 0;

  // Handle wrap-around condition for clock roll-over
  if (a < b) {
    retval = b - a;
  } else {
    retval = (UINT32_MAX - a) + b;
  }

  return retval;
}

void dub_digits(int16_t display_temp) {
  byte digits[4];
  boolean whoops = false;

  // First three digits are temperature reading, so try to make it look right
  // aligned with a negative if needed
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

  // Last digit either unit or error condition
  if (whoops) {
    digits[3] = 0xE;
  } else if (metric_units) {
    digits[3] = 0xC;
  } else {
    digits[3] = 0xF;
  }

  // Copy display info over
  display.setDigits(digits, sizeof(digits));

}

////////////////////////////////////////////////////////////////////////
// INTERRUPT HANDLERS
////////////////////////////////////////////////////////////////////////

ISR(TIMER0_COMPA_vect) {
  display.illuminateNext();
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

  // Setup interrupt handler for display refresh; piggy-backing on TIMER0 that
  // Arudino has already setup for keeping track of time (for millis()).
  // Arduino sets the prescaler to 1/64, so the counter increments every 64
  // clock ticks. At 16MHz, this is around 4us; thus the compare and match
  // register we're setting below should fire once per 256 increments of
  // Timer0's counter, or 1024us.
  OCR0A = 0x7F;
  TIMSK0 |= _BV(OCIE0A);

  // Find and prep temp sensor
  if (!therm.begin()) {
    DPRINTLN(F("Couldn't find sensor!"));
    sensor_error = true;
    // TODO set display to fail mode
  } else {
    curr_temp = readTemp();
  }

  // Setup button debouncing
  butt.attach(BUTT_PIN, INPUT_PULLUP, BUTT_DEBOUNCE_MS);

  // Turn off ADC, SPI
  ADCSRA &= ~(1 << ADEN);
  power_adc_disable();
  power_spi_disable();

}

////////////////////////////////////////////////////////////////////////
// MAIN LOOP
////////////////////////////////////////////////////////////////////////

void loop() {
  
  // Check if temperature needs updating
  uint32_t now = micros();
  if (time_diff(last_check, now) > THERM_POLL_USEC) {
    curr_temp = readTemp();
    //temp_acc += (micros() - now);
    last_check = now;
    //temp_counter++;
    //if (temp_counter >= TEMP_LOOPS) {
    //  DPRINT("temp ");
    //  DPRINTLN(float(temp_acc) / float(temp_counter), 3);
    //  temp_counter = 0;
    //  temp_acc = 0;
    //}
  }

  // Check butt(on) for state change
  if (butt.update() && butt.rose()) {
    metric_units = !metric_units;
  }

  // Update display if needed
  if (curr_temp != last_temp) {
    dub_digits(curr_temp);
    last_temp = curr_temp;
  }

  // Refresh display
  //uint32_t start = micros();
  //display.refreshDisplay(DISP_ON_TIME_USEC);

  // Try to figure how much time the above is using on average
  //usec_acc += (micros() - start);
  //counter++;
  //if (counter >= LOOPS) {
  //  DPRINT("display ");
  //  DPRINTLN(float(usec_acc) / float(counter), 3);
  //  counter = 0;
  //  usec_acc = 0;
  //}

  // Busy loop so the display isn't burnt out
  delayMicroseconds(DISP_OFF_TIME_USEC);

}
