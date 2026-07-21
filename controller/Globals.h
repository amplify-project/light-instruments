#ifndef GLOBALS_H
#define GLOBALS_H

#include <FastLED.h>
#include <vector>
#include <string>

#define LEDSTRIP(port, obj, num_leds) {#port, obj, port, num_leds}

#define LED1 D0
#define LED2 D1
#define LED3 D2
#define LED4 D3

struct LedStrip {
    std::string name;
    CRGB* leds;
    uint8_t port;
    int numLeds;
};

extern std::vector<LedStrip> ledStrips;
#define numLedStrips ((int)ledStrips.size())

void addLedStrip(uint8_t port, int numLeds, const char* name = nullptr);

#endif // GLOBALS_H
