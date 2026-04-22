#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include "display.h"

#define OLED_RST D0

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

    Serial.print("Data: ");
    Serial.println(String(doc["device"]));

    display.setColor(OLEDDISPLAY_COLOR::BLACK);
    display.fillRect(0, 22, 127, 10);

    display.setColor(OLEDDISPLAY_COLOR::WHITE);
    display.drawString(0, 22, "Device: " + String(doc["device"]));
    display.display();

    newPacketReceived = false;
  }
}
