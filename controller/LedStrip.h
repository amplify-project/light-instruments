#ifndef LED_STRIP_H
#define LED_STRIP_H

#include <FastLED.h>
#include <Arduino.h>
#include <string>
#include <memory>

#include "animations/Animation.h"

struct LedStrip {
  std::string name;
  CRGB* leds;
  uint8_t port;
  int numLeds;
  uint8_t brightness = 255;
  CLEDController* controller = nullptr;
  std::unique_ptr<Animation> activeAnimation;

  LedStrip(std::string n, CRGB* l, uint8_t p, int nl, uint8_t b = 255, CLEDController* c = nullptr)
    : name(n), leds(l), port(p), numLeds(nl), brightness(b), controller(c), activeAnimation(nullptr) {}

  void show() {
    if (controller) {
      controller->showLeds(brightness);
    }
  }
};

#endif // LED_STRIP_H
