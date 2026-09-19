#ifndef COMET_ANIMATION_H
#define COMET_ANIMATION_H

#include <Arduino.h>
#include <FastLED.h>

#include "Animation.h"
#include "../Globals.h"

class CometAnimation : public Animation {
public:
  CometAnimation(CRGB color, uint32_t speed)
    : color(color), speed(speed), lastUpdate(millis()), position(0), finished(false) {}

  bool update(int stripIndex) override {
    uint32_t now = millis();

    if (now - lastUpdate < 10) {
      return false;
    }

    uint32_t elapsed = now - lastUpdate;
    lastUpdate = now;

    // Interpret speed as ms per pixel.
    // If speed is 0, we use a very fast rate (e.g. 0.5ms per pixel).
    float pixelsToMove = (float)elapsed / (speed == 0 ? 0.5f : (float)speed);

    // Fade amount proportional to movement to maintain tail appearance.
    // Original behavior was 64 fade per 1 pixel move.
    float fadeAmount = 64.0f * pixelsToMove;
    if (fadeAmount < 1.0f && pixelsToMove > 0) {
      fadeAmount = 1.0f;
    }

    fadeToBlackBy(ledStrips[stripIndex].leds, ledStrips[stripIndex].numLeds, (uint8_t)min(fadeAmount, 255.0f));

    float nextPosition = position + pixelsToMove;
    int start = (int)position;
    int end = (int)nextPosition;

    // Fill all pixels between last and current position to ensure no gaps
    // even at very high speeds.
    for (int i = start; i <= end; i++) {
      if (i >= 0 && i < ledStrips[stripIndex].numLeds) {
        ledStrips[stripIndex].leds[i] = color;
      }
    }

    position = nextPosition;

    if (position >= ledStrips[stripIndex].numLeds + 10) {
      finished = true;
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
  uint32_t lastUpdate;
  float position;
  bool finished;
};

#endif // COMET_ANIMATION_H
