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
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include <Arduino.h>
#include <Adafruit_MCP9808.h>
#include <Bounce2.h>
#include <SevSeg.h>

#include "thermometer.h"

// Temperature sensor variables/consts
Adafruit_MCP9808 therm = Adafruit_MCP9808();
uint32_t last_check = 0;
uint32_t THERM_POLL_MS = 1000; // milliseconds
int curr_temp_c = 0;
int curr_temp_f = 0;
int last_temp_c = 0;
boolean sensor_error = false;
boolean curr_metric_units = false;
boolean last_metric_units = false;

// Display variables/consts
SevSeg display;

const size_t DISP_DIGITS = 4;
const size_t DISP_BRIGHT = 192;
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

// Button variables/consts
Bounce butt;
const byte BUTT_PIN = 9;
const uint16_t BUTT_DEBOUNCE_MS = 50;

////////////////////////////////////////////////////////////////////////
// HALPING
////////////////////////////////////////////////////////////////////////

void readTemp() {
  
  if (sensor_error) {
    return;
  }

  //therm.shutdown_wake(0);
  curr_temp_c = therm.readTempC();
  //therm.shutdown_wake(1);

  DPRINT("tempC="); DPRINT(curr_temp_c);

  // Convert to Fahrenheit because I suck and can't read Celcius well (yet)
  if (!curr_metric_units) {
    curr_temp_f = curr_temp_c * 9.0 / 5.0 + 32;
  }

  DPRINT(" tempF="); DPRINTLN(curr_temp_f);

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

void dub_digits() {
  int display_temp;
  byte digits[4];

  // Last digit either is the unit, and incidentally which one are we
  // displaying?
  if (curr_metric_units) {
    digits[3] = 0xC;
    display_temp = curr_temp_c;
  } else {
    digits[3] = 0xF;
    display_temp = curr_temp_f;
  }

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
    digits[0] = 0xE;
    digits[1] = 0xE;
    digits[2] = 0xE;
    digits[3] = 0xE;
  }

  // Copy display info over
  display.setDigits(digits, sizeof(digits));

}

/*
 * sleep_now()
 *
 * Sends CPU to sleep until an interrupt or the configured watchdog timer goes
 * off.
 */
void sleep_now() {

  wdt_enable(WDTO_1S);
  WDTCSR |= (1 << WDIE);

  set_sleep_mode(SLEEP_MODE_IDLE);
  cli();
  if (1) {
    sleep_enable();
    //sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
  sei();

}

////////////////////////////////////////////////////////////////////////
// INTERRUPT HANDLERS
////////////////////////////////////////////////////////////////////////

ISR (TIMER0_COMPA_vect) {
  display.illuminateNext();
}

ISR (WDT_vect) {
  wdt_disable();
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
  display.setBrightness(DISP_BRIGHT);

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
    readTemp();
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
  uint32_t now = millis();
  if (time_diff(last_check, now) > THERM_POLL_MS) {
    readTemp();
    last_check = now;
  }

  // Check butt(on) for state change
  if (butt.update() && butt.rose()) {
    last_metric_units = curr_metric_units;
    curr_metric_units = !curr_metric_units;
  }

  // Update display if needed
  if (curr_temp_c != last_temp_c || curr_metric_units != last_metric_units) {
    dub_digits();
    last_temp_c = curr_temp_c;
  }

  // Sleep for 1 s with most non-GPIO stuff off
  //sleep_now();

}
