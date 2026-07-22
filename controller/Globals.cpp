#include "Globals.h"
#include "CommandManager.h"
#include <Arduino.h>

std::vector<LedStrip> ledStrips;
CommandManager commandManager;

void showStrip(int index) {
  if (index >= 0 && index < numLedStrips) {
    ledStrips[index].show();
  }
}

void showAll() {
  for (int i = 0; i < numLedStrips; i++) {
    ledStrips[i].show();
  }
}

void addLedStrip(uint8_t port, int numLeds, const char* name) {
  CRGB* leds = new CRGB[numLeds];
  std::string stripName = name ? name : "D" + std::to_string(port);
  CLEDController* controller = nullptr;

  switch (port) {
    case LED1:
      controller = &FastLED.addLeds<WS2812B, LED1, GRB>(leds, numLeds);
      break;
    case LED2:
      controller = &FastLED.addLeds<WS2812B, LED2, GRB>(leds, numLeds);
      break;
    case LED3:
      controller = &FastLED.addLeds<WS2812B, LED3, GRB>(leds, numLeds);
      break;
    case LED4:
      controller = &FastLED.addLeds<WS2812B, LED4, GRB>(leds, numLeds);
      break;
  }

  ledStrips.emplace_back(stripName, leds, port, numLeds, (uint8_t)255, controller);
}
