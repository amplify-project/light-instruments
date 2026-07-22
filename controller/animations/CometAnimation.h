#ifndef COMET_ANIMATION_H
#define COMET_ANIMATION_H

#include "Animation.h"
#include "../Globals.h"
#include <Arduino.h>
#include <FastLED.h>

class CometAnimation : public Animation {
public:
  CometAnimation(CRGB color, uint32_t speed)
    : color(color), speed(speed), lastUpdateLogic(millis()), lastUpdateDither(0), position(0), finished(false) {}

  bool update(int stripIndex) override {
    uint32_t now = millis();

    if (now - lastUpdateDither < 10) {
      return false;
    }

    lastUpdateDither = now;

    if (now - lastUpdateLogic >= speed) {
      lastUpdateLogic = now;

      fadeToBlackBy(ledStrips[stripIndex].leds, ledStrips[stripIndex].numLeds, 64);

      if (position < ledStrips[stripIndex].numLeds) {
        ledStrips[stripIndex].leds[(int)position] = color;
        position += 1.0f;
      } else if (position < ledStrips[stripIndex].numLeds + 10) {
        position += 1.0f;
      } else {
        finished = true;
      }
    }

    return true;
  }

  bool isFinished() override {
    return finished;
  }

  void setColor(CRGB newColor) override {
    color = newColor;
  }

private:
  CRGB color;
  uint32_t speed;
  uint32_t lastUpdateLogic;
  uint32_t lastUpdateDither;
  float position;
  bool finished;
};

#endif // COMET_ANIMATION_H
