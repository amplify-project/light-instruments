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
extern int numLedsPerStrip[4];
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

  int legacyNumLeds = prefs.getInt("numleds", 0);

  for (int i = 0; i < 4; i++) {
    String key = "numleds_" + String(i);
    numLedsPerStrip[i] = prefs.getInt(key.c_str(), legacyNumLeds);
  }

  numStrips = prefs.getInt("numstrips", 0);
  prefs.end();

  bool hasValidLeds = true;

  if (numStrips > 0) {
    for (int i = 0; i < numStrips; i++) {
      if (numLedsPerStrip[i] <= 0) {
        hasValidLeds = false;
        break;
      }
    }
  } else {
    hasValidLeds = false;
  }

  return (deviceName != "" && hasValidLeds && numStrips > 0);
}

void saveDeviceName(String name) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putString("name", name);
  prefs.end();

  deviceName = name;
}

void saveNumLeds(int numLeds, int index) {
  Preferences prefs;
  prefs.begin("system", false);

  if (index >= 0 && index < 4) {
    String key = "numleds_" + String(index);
    prefs.putInt(key.c_str(), numLeds);
    numLedsPerStrip[index] = numLeds;
  } else {
    // Set all strips (index = -1)
    prefs.putInt("numleds", numLeds);
    for (int i = 0; i < 4; i++) {
      String key = "numleds_" + String(i);
      prefs.putInt(key.c_str(), numLeds);
      numLedsPerStrip[i] = numLeds;
    }
  }

  prefs.end();
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

    if (input == "resetconfig") {
      Preferences prefs;
      prefs.begin("system", false);
      prefs.clear();
      prefs.end();

      Serial.println("Configuration cleared. Rebooting in 1 second...");
      vTaskDelay(pdMS_TO_TICKS(1000));
      ESP.restart();
    } else if (input == "reboot") {
      Serial.println("Rebooting...");
      vTaskDelay(pdMS_TO_TICKS(500));
      ESP.restart();
    } else if (input == "status") {
      Serial.println("--- Current Configuration ---");
      Serial.print("Device Name: ");
      Serial.println(deviceName == "" ? "[Not set]" : deviceName);

      Serial.println("LED counts:");

      for (int i = 0; i < numStrips; i++) {
        Serial.print("  Strip ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(numLedsPerStrip[i]);
      }

      Serial.print("Number of active strips: ");
      Serial.println(numStrips);
      Serial.println("-----------------------------");
    } else if (input.startsWith("name=")) {
      String newName = input.substring(5);

      if (newName.length() > 0) {
        saveDeviceName(newName);

        Serial.print("Device name updated and saved to flash: ");
        Serial.println(deviceName);
      }
    } else if (input.startsWith("numleds")) {
      int index = -1;
      int valueStart = 8;

      if (input.length() > 8 && input.charAt(7) >= '0' && input.charAt(7) <= '3' && input.charAt(8) == '=') {
        index = input.charAt(7) - '0';
        valueStart = 9;
      } else if (input.startsWith("numleds=")) {
        index = -1;
        valueStart = 8;
      } else {
        return false;
      }

      int newNumLeds = input.substring(valueStart).toInt();

      if (newNumLeds > 0) {
        saveNumLeds(newNumLeds, index);

        Serial.print("Number of LEDs for ");
        if (index >= 0) {
          Serial.print("strip ");
          Serial.print(index);
        } else {
          Serial.print("all strips");
        }
        Serial.print(" updated to ");
        Serial.println(newNumLeds);
        Serial.println("Reboot required to apply changes.");
      }
    } else if (input.startsWith("numstrips=")) {
      int newNumStrips = input.substring(10).toInt();

      if (newNumStrips > 0 && newNumStrips <= 4) {
        saveNumStrips(newNumStrips);

        Serial.print("Number of LED strips updated and saved to flash: ");
        Serial.println(numStrips);
        Serial.println("Reboot required to apply changes.");
      }
    }

    bool hasValidLeds = true;

    if (numStrips > 0) {
      for (int i = 0; i < numStrips; i++) {
        if (numLedsPerStrip[i] <= 0) {
          hasValidLeds = false;
          break;
        }
      }
    } else {
      hasValidLeds = false;
    }

    return (deviceName != "" && hasValidLeds && numStrips > 0);
  }

  return false;
}
