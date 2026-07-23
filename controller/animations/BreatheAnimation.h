#ifndef BREATHE_ANIMATION_H
#define BREATHE_ANIMATION_H

#include <Arduino.h>
#include <FastLED.h>

#include "Animation.h"
#include "../Globals.h"

class BreatheAnimation : public Animation {
public:
  BreatheAnimation(CRGB color, uint8_t bpm)
    : color(color), bpm(bpm), lastUpdate(0) {}

  bool update(int stripIndex) override {
    uint32_t now = millis();

    if (now - lastUpdate < 10) {
      return false;
    }

    lastUpdate = now;

    uint16_t b16 = beatsin16(bpm, 0, 65535);

    CRGB scaledColor = color;
    scaledColor.nscale8_video(b16 >> 8);
    fill_solid(ledStrips[stripIndex].leds, ledStrips[stripIndex].numLeds, scaledColor);

    return true;
  }

  bool isFinished() override {
    return false; // Breathe runs indefinitely
  }

  void setColor(CRGB newColor) override {
    color = newColor;
  }

private:
  CRGB color;
  uint8_t bpm;
  uint32_t lastUpdate;
};

#endif // BREATHE_ANIMATION_H
