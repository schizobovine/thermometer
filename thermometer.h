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
// DEBUGGING
////////////////////////////////////////////////////////////////////////

#define SERIAL_DEBUG 0
#define SERIAL_BAUD 9600

#ifdef SERIAL_DEBUG
#define DPRINT(...) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINTLN(...)
#endif

#endif
