#ifndef PULSE_ANIMATION_H
#define PULSE_ANIMATION_H

#include "Animation.h"
#include "../Globals.h"
#include <Arduino.h>
#include <FastLED.h>

class PulseAnimation : public Animation {
public:
    PulseAnimation(CRGB color, uint16_t a, uint16_t d, uint16_t s, uint16_t re)
        : color(color), attack(a), decay(d), sustain(s), release(re), startTime(millis()), lastUpdate(0), finished(false) {}

    bool update(int stripIndex) override {
        uint32_t now = millis();

        if (now - lastUpdate < 20) {
            return false;
        }

        lastUpdate = now;

        uint32_t elapsed = now - startTime;
        uint8_t brightness = 0;

        uint32_t attackEnd = attack;
        uint32_t decayEnd = attackEnd + decay;
        uint32_t sustainEnd = decayEnd + sustain;
        uint32_t releaseEnd = sustainEnd + release;

        if (elapsed < attackEnd) {
            brightness = (attack > 0) ? map(elapsed, 0, attack, 0, 255) : 255;
        } else if (elapsed < decayEnd) {
            brightness = (decay > 0) ? map(elapsed - attackEnd, 0, decay, 255, 200) : 200;
        } else if (elapsed < sustainEnd) {
            brightness = 200;
        } else if (elapsed < releaseEnd) {
            brightness = (release > 0) ? map(elapsed - sustainEnd, 0, release, 200, 0) : 0;
        } else {
            finished = true;
            brightness = 0;
        }

        CRGB scaledColor = color;
        scaledColor.nscale8_video(brightness);
        fill_solid(ledStrips[stripIndex].leds, ledStrips[stripIndex].numLeds, scaledColor);

        return true;
    }

    bool isFinished() override {
        return finished;
    }

private:
    CRGB color;
    uint16_t attack, decay, sustain, release;
    uint32_t startTime;
    uint32_t lastUpdate;
    bool finished;
};

#endif // PULSE_ANIMATION_H
