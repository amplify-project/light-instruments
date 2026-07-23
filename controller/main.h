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
#include <FastLED.h>

#include "Globals.h"
#include "CommandManager.h"
#include "Protocol.h"

String deviceName = "";
String deviceType = "actuator";

struct Packet {
  uint8_t mac[6];
  uint8_t data[250];
  int len;
};

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
bool pingReceived = false;
std::mutex pingMtx;

std::deque<Packet> packetQueue;
const size_t MAX_QUEUE_SIZE = 20;
std::mutex queueMtx;

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len < (int)sizeof(ProtocolHeader)) return;
  ProtocolHeader* header = (ProtocolHeader*)incomingData;

  if (header->type == MSG_DISCOVERY) {
    if (!relayFound) {
      memcpy(relayAddress, mac, 6);
      relayFound = true;
    }
  } else if (header->type == MSG_PING) {
    std::lock_guard<std::mutex> lock(pingMtx);
    pingReceived = true;
  } else if (header->type == MSG_COMMAND) {
    // Protect the queue with a mutex since this callback runs in a different task context
    std::lock_guard<std::mutex> lock(queueMtx);

    if (packetQueue.size() < MAX_QUEUE_SIZE) {
      // Create a packet structure to store data and MAC
      Packet p;
      memcpy(p.mac, mac, 6);
      p.len = (len > 250) ? 250 : len;
      memcpy(p.data, incomingData, p.len);

      packetQueue.push_back(p);
    } else {
      Serial.println("Packet queue full, dropping packet");
    }
  }
}

void sendDiscoveryResponse() {
  DiscoveryResponsePacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DISCOVERY_RESPONSE;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.deviceType, deviceType.c_str(), sizeof(packet.deviceType) - 1);

  esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
}

void sendPong() {
  PongPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_PONG;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.deviceType, deviceType.c_str(), sizeof(packet.deviceType) - 1);

  {
    std::lock_guard<std::mutex> lock(pingMtx);
    pingReceived = false;
  }
  esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
}

void handlePing() {
  bool shouldPong = false;

  {
    std::lock_guard<std::mutex> lock(pingMtx);
    shouldPong = pingReceived;
  }

  if (shouldPong) {
    sendPong();
  }
}

void processIncomingPackets() {
  Packet currentPacket;
  bool hasPacket = false;

  {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (!packetQueue.empty()) {
      currentPacket = packetQueue.front();
      packetQueue.pop_front();
      hasPacket = true;
    }
  }

  if (!hasPacket) {
    return;
  }

  if (currentPacket.len < (int)sizeof(CommandPacket)) {
    return;
  }

  CommandPacket* p = (CommandPacket*)currentPacket.data;
  commandManager.process(*p);
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
