#ifndef GLOBALS_H
#define GLOBALS_H

#include <FastLED.h>

#define NUM_LEDS_D0 30
#define DATA_PIN_D0 D0

extern CRGB ledsD0[NUM_LEDS_D0];

struct LedStrip {
    const char* name;
    CRGB* leds;
    int numLeds;
};

extern LedStrip ledStrips[];
extern const int numLedStrips;

#endif // GLOBALS_H
