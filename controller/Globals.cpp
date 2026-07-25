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
extern int numLedsPerStrip;
extern int numStrips;

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

bool initPersistentConfig() {
  Preferences prefs;
  prefs.begin("system", true);
  deviceName = prefs.getString("name", "");
  numLedsPerStrip = prefs.getInt("numleds", 0);
  numStrips = prefs.getInt("numstrips", 0);
  prefs.end();

  return (deviceName != "" && numLedsPerStrip > 0 && numStrips > 0);
}

void saveDeviceName(String name) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putString("name", name);
  prefs.end();

  deviceName = name;
}

void saveNumLeds(int numLeds) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putInt("numleds", numLeds);
  prefs.end();

  numLedsPerStrip = numLeds;
}

void saveNumStrips(int numStripsValue) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putInt("numstrips", numStripsValue);
  prefs.end();

  numStrips = numStripsValue;
}

bool listenForSerialConfig() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("name=")) {
      String newName = input.substring(5);

      if (newName.length() > 0) {
        saveDeviceName(newName);

        Serial.print("Device name updated and saved to flash: ");
        Serial.println(deviceName);
      }
    } else if (input.startsWith("numleds=")) {
      int newNumLeds = input.substring(8).toInt();

      if (newNumLeds > 0) {
        saveNumLeds(newNumLeds);

        Serial.print("Number of LEDs updated and saved to flash: ");
        Serial.println(numLedsPerStrip);
      }
    } else if (input.startsWith("numstrips=")) {
      int newNumStrips = input.substring(10).toInt();

      if (newNumStrips > 0) {
        saveNumStrips(newNumStrips);

        Serial.print("Number of LED strips updated and saved to flash: ");
        Serial.println(numStrips);
      }
    }

    return (deviceName != "" && numLedsPerStrip > 0 && numStrips > 0);
  }

  return false;
}
