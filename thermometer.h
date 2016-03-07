/*
 * thermometer.h
 *
 * Simple 7-segment display thermometer.
 *
 * Author: Sean Caulfield <sean@yak.net>
 * License: GPL v2.0
 *
 */

#ifndef __THERMOMETER_H
#define __THERMOMETER_H

////////////////////////////////////////////////////////////////////////
// MACRO (POLO)
////////////////////////////////////////////////////////////////////////

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

#endif
