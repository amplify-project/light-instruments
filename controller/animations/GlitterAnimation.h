#ifndef GLITTER_ANIMATION_H
#define GLITTER_ANIMATION_H

#include "Animation.h"
#include "../Globals.h"
#include <Arduino.h>
#include <FastLED.h>

class GlitterAnimation : public Animation {
public:
  GlitterAnimation(CRGB color, uint32_t duration)
    : color(color), duration(duration), startTime(millis()), lastUpdate(millis() - 20), finished(false) {}

  bool update(int stripIndex) override {
    uint32_t now = millis();
    uint32_t elapsed = now - startTime;

    if (duration > 0 && elapsed >= duration) {
      finished = true;
      return false;
    }

    if (now - lastUpdate >= 20) {
      lastUpdate = now;
      fadeToBlackBy(ledStrips[stripIndex].leds, ledStrips[stripIndex].numLeds, 32);

      if (random8() < 64) {
        int pos = random16(ledStrips[stripIndex].numLeds);
        ledStrips[stripIndex].leds[pos] += color;
      }

      return true;
    }

    return false;
  }

  bool isFinished() override {
    return finished;
  }

  void setColor(CRGB newColor) override {
    color = newColor;
  }

private:
  CRGB color;
  uint32_t duration;
  uint32_t startTime;
  uint32_t lastUpdate;
  bool finished;
};

#endif // GLITTER_ANIMATION_H
