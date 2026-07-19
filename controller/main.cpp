#include <deque>
#include <mutex>
#include <memory>

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include "Globals.h"
#include "Commands.h"
#include "CommandManager.h"

struct Packet {
  uint8_t mac[6];
  JsonDocument doc;
};

// Definitions of globals declared in Globals.h
CRGB ledsD0[NUM_LEDS_D0];

LedStrip ledStrips[] = {
    {"D0", ledsD0, NUM_LEDS_D0}
};
const int numLedStrips = sizeof(ledStrips) / sizeof(LedStrip);

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;

String deviceName = "receiver1";
String deviceType = "actuator";

bool pingReceived = false;

std::deque<Packet> packetQueue;
const size_t MAX_QUEUE_SIZE = 20;
std::mutex queueMtx;

CommandManager commandManager;

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

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);

  FastLED.addLeds<WS2812B, DATA_PIN_D0, GRB>(ledsD0, NUM_LEDS_D0);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();

  commandManager.registerCommand("set", std::unique_ptr<SetCommand>(new SetCommand()));
  commandManager.registerCommand("setColor", std::unique_ptr<LightCommand>(new SetColorCommand()));
  commandManager.registerCommand("pulse", std::unique_ptr<LightCommand>(new PulseCommand()));
  commandManager.registerCommand("comet", std::unique_ptr<LightCommand>(new CometCommand()));
  commandManager.registerCommand("glitter", std::unique_ptr<LightCommand>(new GlitterCommand()));
  commandManager.registerCommand("rainbow", std::unique_ptr<LightCommand>(new RainbowCommand()));
  commandManager.registerCommand("setBrightness", std::unique_ptr<LightCommand>(new SetBrightnessCommand()));

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
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (pingReceived) {
    sendPong();
  }

  processIncomingPackets();
  commandManager.update();
}
