#ifndef RAINBOW_ANIMATION_H
#define RAINBOW_ANIMATION_H

#include "Animation.h"
#include "../Globals.h"
#include <Arduino.h>
#include <FastLED.h>

class RainbowAnimation : public Animation {
public:
    RainbowAnimation(uint8_t deltaHue) : deltaHue(deltaHue), lastUpdate(0), initialHue(0) {}

    bool update(int stripIndex) override {
        uint32_t now = millis();

        if (now - lastUpdate >= 20) {
            lastUpdate = now;
            initialHue++;
            fill_rainbow(ledStrips[stripIndex].leds, ledStrips[stripIndex].numLeds, initialHue, deltaHue);

            return true;
        }

        return false;
    }

    bool isFinished() override {
        return false;
    }

private:
    uint8_t deltaHue;
    uint32_t lastUpdate;
    uint8_t initialHue;
};

#endif // RAINBOW_ANIMATION_H
