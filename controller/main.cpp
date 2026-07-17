#include <deque>
#include <mutex>

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#define NUM_LEDS_D0 30
#define DATA_PIN_D0 D0

struct Packet {
  uint8_t mac[6];
  JsonDocument doc;
};

CRGB ledsD0[NUM_LEDS_D0];

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;

String deviceName = "receiver1";
String deviceType = "actuator";

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

void processLightCommand(const JsonDocument& doc) {
    const char* port = doc["port"];

    if (doc["command"] == "setColor") {
      const char* value = doc["value"];

      if (value) {
        // If no port is specified, default to D0, or check if it matches D0
        if (port == nullptr || strcmp(port, "D0") == 0) {
          int r, g, b;

          if (sscanf(value, "%d,%d,%d", &r, &g, &b) == 3) {
            fill_solid(ledsD0, NUM_LEDS_D0, CRGB(r, g, b));
            FastLED.show();
          }
        }
      }
    } else if (doc["command"] == "setBrightness") {
      const char* value = doc["value"];

      if (value) {
        if (port == nullptr || strcmp(port, "D0") == 0) {
          FastLED.setBrightness(atoi(value));
          FastLED.show();
        }
      }
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
    processLightCommand(currentPacket.doc);
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
}
