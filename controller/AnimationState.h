#ifndef ANIMATION_STATE_H
#define ANIMATION_STATE_H

#include <FastLED.h>

struct AnimationState {
    bool active = false;
    char targetPort[16] = ""; // Empty string means all ports
    uint32_t startTime = 0;
    uint16_t attack = 0;  // ms
    uint16_t decay = 0;   // ms
    uint16_t sustain = 0; // ms
    uint16_t release = 0; // ms
    uint8_t targetBrightness = 255;
    CRGB color = CRGB::White;
};

#endif // ANIMATION_STATE_H
