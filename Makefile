ARDUINO_DIR       = $(realpath $(HOME)/arduino/arduino-current)
ARDUINO_LIBS      := Wire Adafruit_MCP9808 SevSeg

ARDUINO_PORT      := /dev/ttyUSB0
USER_LIB_PATH     := $(realpath ./lib)

ARCHITECTURE      = avr
#ALTERNATE_CORE    = adafruit
BOARD_TAG         = uno

#CXXFLAGS      += -std=gnu++11 -Wl,-u,vfprintf
#CFLAGS        += -std=gnu11 -Wl,-u,vfprintf
#LDFLAGS       += -lprintf_flt -lm -Wl,-u,vfprintf
CXXFLAGS      += -std=gnu++11 -fno-threadsafe-statics
CFLAGS        += -std=gnu11

include Arduino.mk
