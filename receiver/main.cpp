#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include "display.h"

#define OLED_RST D0
#define LED_COUNT 35
#define LED_PIN D3

const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,
  8,  9,  9,  9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15,
  15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 23, 23,
  24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35,
  35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
  50, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66,
  67, 68, 69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86,
  87, 89, 90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,
  112,114,115,117,119,120,122,124,126,127,129,131,133,135,137,138,
  140,142,144,146,148,150,152,154,156,158,160,162,164,167,169,171,
  173,175,177,180,182,184,186,189,191,193,196,198,200,203,205,208,
  210,213,215,218,220,223,225,228,231,233,236,239,241,244,247,249,
  252,255
};

float brightness = 0.5;

// Initialise NeoPixel LED strip
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

volatile bool newPacketReceived = false;
char dataBuffer[512];
Display display(SDA, SCL, OLED_RST, GEOMETRY_128_32);

void initScreen() {
  display.init();
  display.flipScreenVertically();
  display.clear();
}

void onReceive(const uint8_t *macAddr, const uint8_t *data, int len) {
  memcpy(dataBuffer, data, len);
  newPacketReceived = true;
}

void updateScreen(JsonDocument &doc) {
  Serial.print("Received: ");
  Serial.print(String(doc["device"]));
  Serial.print(" ");
  Serial.print(String(doc["port"]));
  Serial.print(" ");
  Serial.println(String(doc["data"]));

  display.setColor(OLEDDISPLAY_COLOR::BLACK);
  display.fillRect(0, 22, 127, 10);

  display.setColor(OLEDDISPLAY_COLOR::WHITE);
  display.drawString(0, 22, String(doc["device"] + " => " + String(doc["data"])));
  display.display();
}

void updateLights(JsonDocument doc) {
  if (String(doc["device"]).equals("keys")) {
    if (doc["data"] == 1) {
      Serial.println("Setting brightness to 50");
      strip.setBrightness(50);

      if (doc["port"] == 3) {
        Serial.println("Setting colour red");

        for (int i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(255, 0, 0));
        }
      } else if (doc["port"] == 2) {
        Serial.println("Setting colour green");

        for (int i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0, 255, 0));
        }
      } else {
        Serial.println("Setting colour blue");

        for (int i=0; i<strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0, 0, 255));
        }
      }
    } else {
      Serial.println("Setting brightness to 0");
      strip.setBrightness(0);
    }

    Serial.println("Update strip");
    strip.show();
  } else if (String(doc["device"]).equals("touch")) {
    strip.setBrightness(map(doc["data"], 0, 4096, 0, 255));
    strip.show();
  } else if (String(doc["device"]).equals("rattle")) {
    uint8_t prevBrightness = strip.getBrightness();

    strip.setBrightness(200);
    strip.show();

    delay(50);
    strip.setBrightness(prevBrightness);
    strip.show();
  } else if (String(doc["device"]).equals("percussion_big")) {
    int adjustedValue = doc["data"];
    Serial.printf("vibration %d\n", adjustedValue);

    if (adjustedValue < 50) {
      strip.setBrightness(50);
    } else if (adjustedValue > 255){
      strip.setBrightness(map(adjustedValue, 0, 4096, 0, 255));
    } else {
      strip.setBrightness(adjustedValue);
    }

    strip.show();
  } else if (String(doc["device"]).equals("percussion_small")) {
    int adjustedValue = map(doc["data"], 0, 4096, 0, 254);
    Serial.printf("vibration %d\n", adjustedValue);

    strip.setBrightness(adjustedValue);
    strip.show();
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Could not initialise ESP Now");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(onReceive));
  Serial.println("ESP Now callback registered. Waiting for messages...");

  initScreen();

  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "MAC Address:");
  display.drawString(0, 10, WiFi.macAddress());
  display.display();

  // Initialise LED strip and set it to medium brightness
  strip.begin();
  strip.show();
  strip.setBrightness(50);

  // Set colour of all LEDs to rgb(255, 0, 0) (red)
  for (int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }

  // Update LED strip
  strip.show();
}

void loop() {
  if (newPacketReceived) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, dataBuffer);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      return;
    }

    updateLights(doc);
    updateScreen(doc);

    newPacketReceived = false;
  }
}
