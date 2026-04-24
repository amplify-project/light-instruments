#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include "display.h"

#define OLED_RST D0
#define LED_COUNT 35
#define LED_PIN D3

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
