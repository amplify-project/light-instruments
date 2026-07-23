#include <Arduino.h>
#include <Preferences.h>
#include <esp_mac.h>

#include "Globals.h"
#include "CommandManager.h"

std::vector<LedStrip> ledStrips;
CommandManager commandManager;
TaskHandle_t displayTaskHandle = NULL;
extern String deviceName;
extern String deviceType;

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

void triggerDisplay() {
  if (displayTaskHandle != NULL) {
    xTaskNotifyGive(displayTaskHandle);
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

bool initDeviceName() {
  Preferences prefs;
  prefs.begin("system", true);
  deviceName = prefs.getString("name", "");
  prefs.end();

  if (deviceName == "") {
    return false;
  }

  return true;
}

void saveDeviceName(String name) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putString("name", name);
  prefs.end();

  deviceName = name;
}

bool listenForDeviceName() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("name=")) {
      String newName = input.substring(5);

      if (newName.length() > 0) {
        saveDeviceName(newName);

        Serial.print("Device name updated and saved to flash: ");
        Serial.println(deviceName);
        return true;
      }
    }
  }

  return false;
}
