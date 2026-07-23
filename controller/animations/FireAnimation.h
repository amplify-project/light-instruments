#ifndef FIRE_ANIMATION_H
#define FIRE_ANIMATION_H

#include "Animation.h"
#include "../Globals.h"
#include <Arduino.h>
#include <FastLED.h>
#include <vector>

class FireAnimation : public Animation {
public:
  FireAnimation(CRGB color, uint8_t intensity)
    : color(color), intensity(intensity), lastUpdate(0) {}

  bool update(int stripIndex) override {
    uint32_t now = millis();

    if (now - lastUpdate < 10) {
      return false;
    }

    lastUpdate = now;

    int numLeds = ledStrips[stripIndex].numLeds;
    CRGB* leds = ledStrips[stripIndex].leds;

    if (heat.size() != (size_t)numLeds) {
      heat.assign(numLeds, 0);
    }

    // Step 1. Cool down every cell a little bit
    for (int i = 0; i < numLeds; i++) {
      heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / numLeds) + 2));
    }

    // Step 2. Heat from each cell drifts 'up' and diffuses a little
    for (int k = numLeds - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3. Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < intensity) {
      int y = random8(7);
      heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4. Map from heat cells to LED colors
    for (int j = 0; j < numLeds; j++) {
      leds[j] = color;
      leds[j].nscale8_video(heat[j]);
    }

    return true;
  }

  bool isFinished() override {
    return false;
  }

  void setColor(CRGB newColor) override {
    color = newColor;
  }

private:
  CRGB color;
  uint8_t intensity;
  uint32_t lastUpdate;
  std::vector<uint8_t> heat;
};

#endif // FIRE_ANIMATION_H
