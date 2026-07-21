#ifndef MAIN_H
#define MAIN_H

#include <deque>
#include <mutex>
#include <memory>
#include <vector>
#include <string>

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "Globals.h"
#include "CommandManager.h"

extern String deviceName;
String deviceType = "actuator";

struct Packet {
  uint8_t mac[6];
  JsonDocument doc;
};

inline void addLedStrip(uint8_t port, int numLeds, const char* name) {
  CRGB* leds = new CRGB[numLeds];
  std::string stripName = name ? name : "D" + std::to_string(port);
  CLEDController* controller = nullptr;

  switch(port) {
    case LED1: controller = &FastLED.addLeds<WS2812B, LED1, GRB>(leds, numLeds); break;
    case LED2: controller = &FastLED.addLeds<WS2812B, LED2, GRB>(leds, numLeds); break;
    case LED3: controller = &FastLED.addLeds<WS2812B, LED3, GRB>(leds, numLeds); break;
    case LED4: controller = &FastLED.addLeds<WS2812B, LED4, GRB>(leds, numLeds); break;
  }

  ledStrips.emplace_back(stripName, leds, port, numLeds, (uint8_t)255, controller);
}

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
bool pingReceived = false;

std::deque<Packet> packetQueue;
const size_t MAX_QUEUE_SIZE = 20;
std::mutex queueMtx;

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, incomingData, len);

  if (!error) {
    if (doc["command"] == "discovery") {
      if (!relayFound) {
        memcpy(relayAddress, mac, 6);
        relayFound = true;
      }
    } else if (doc["command"] == "ping") {
      pingReceived = true;
    } else {
      // Protect the queue with a mutex since this callback runs in a different task context
      std::lock_guard<std::mutex> lock(queueMtx);

      if (packetQueue.size() < MAX_QUEUE_SIZE) {
        // Create a packet structure to store data and MAC
        Packet p;
        memcpy(p.mac, mac, 6);
        p.doc = std::move(doc); // Use move to avoid copying the JsonDocument

        packetQueue.push_back(std::move(p));
      } else {
        Serial.println("Packet queue full, dropping packet");
      }
    }
  }
}

void sendDiscoveryResponse() {
  JsonDocument doc;
  doc["command"] = "discoveryResponse";
  doc["deviceType"] = deviceType;
  doc["device"] = deviceName;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_now_send(relayAddress, (uint8_t *) buffer, strlen(buffer) + 1);
}

void sendPong() {
  JsonDocument doc;
  doc["command"] = "pong";
  doc["deviceType"] = deviceType;
  doc["device"] = deviceName;

  char buffer[128];
  serializeJson(doc, buffer);

  pingReceived = false;
  esp_now_send(relayAddress, (uint8_t *)buffer, strlen(buffer) + 1);
}

void handlePing() {
  if (pingReceived) {
    sendPong();
  }
}

void processIncomingPackets() {
  Packet currentPacket;
  bool hasPacket = false;

  {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (!packetQueue.empty()) {
      currentPacket = std::move(packetQueue.front()); // Move out of the queue
      packetQueue.pop_front(); // Efficient removal from deque
      hasPacket = true;
    }
  }

  if (!hasPacket) {
    return;
  }

  // Packet is a command packet if the key 'command' is set
  if (!currentPacket.doc["command"].isNull()) {
    const char* command = currentPacket.doc["command"];
    const char* port = currentPacket.doc["port"];
    const char* data = currentPacket.doc["data"];

    Serial.printf("%s %s %s\n", command, port, data);

    commandManager.process(currentPacket.doc);
  }
}

void flashBuiltinLed() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void setupWireless() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Waiting for relay discovery...");
  while (!relayFound) {
    delay(10);
  }
  Serial.println("Relay discovered!");

  // Register Peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, relayAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  sendDiscoveryResponse();
}

#endif // MAIN_H
