#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_now.h>
#include <vector>
#include <mutex>
#include <map>
#include <array>

struct Packet {
  uint8_t mac[6];
  String data;
};

std::vector<Packet> packetQueue;
std::mutex queueMtx;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

std::map<String, std::array<uint8_t, 6>> discoveredDevices;
unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 10000; // 10 seconds

volatile unsigned long ledFlashTime = 0;
const int FLASH_DURATION = 50;

void onReceive(const uint8_t *macAddr, const uint8_t *data, int len) {
  // Create a packet structure to store data and MAC
  Packet p;
  memcpy(p.mac, macAddr, 6);
  p.data = String((const char*)data, len);

  // Protect the queue with a mutex since this callback runs in a different task context
  std::lock_guard<std::mutex> lock(queueMtx);
  packetQueue.push_back(p);

  // Flash LED for activity
  digitalWrite(LED_BUILTIN, HIGH); // Turn OFF (active low)
  ledFlashTime = millis();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    // Could not initialise ESP Now
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(onReceive));

  // Add broadcast peer for sending commands
  esp_now_peer_info_t peerInfo = {};
  memset(&peerInfo, 0, sizeof(peerInfo));

  for (int i = 0; i < 6; i++) {
    peerInfo.peer_addr[i] = 0xFF;
  }

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    // Failed to add broadcast peer
    return;
  }

  // Turn on builtin LED to indicate successful initialization
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Send initial discovery message
  JsonDocument discoveryDoc;
  discoveryDoc["command"] = "discovery";
  char buffer[128];
  serializeJson(discoveryDoc, buffer);
  esp_now_send(broadcastAddress, (uint8_t *)buffer, strlen(buffer) + 1);
  lastDiscoveryTime = millis();
}

void loop() {
  // Handle LED flash timer
  if (ledFlashTime > 0 && millis() - ledFlashTime > FLASH_DURATION) {
    digitalWrite(LED_BUILTIN, LOW); // Turn back ON (Ready state)
    ledFlashTime = 0;
  }

  // Handle discovery broadcast
  if (millis() - lastDiscoveryTime > DISCOVERY_INTERVAL) {
    lastDiscoveryTime = millis();
    JsonDocument doc;
    doc["command"] = "discovery";
    char buffer[128];
    serializeJson(doc, buffer);
    esp_now_send(broadcastAddress, (uint8_t *)buffer, strlen(buffer) + 1);

    // Flash LED for activity
    digitalWrite(LED_BUILTIN, HIGH); // Turn OFF (active low)
    ledFlashTime = millis();
  }

  // Handle ESP Now -> Serial
  Packet currentPacket;
  bool hasPacket = false;

  {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (!packetQueue.empty()) {
      currentPacket = packetQueue.front();
      packetQueue.erase(packetQueue.begin());
      hasPacket = true;
    }
  }

  if (hasPacket) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, currentPacket.data);

    if (!error) {
      const char* device = doc["device"];

      if (device) {
        if (!doc["port"].isNull() && !doc["data"].isNull()) {
          const char* port = doc["port"];
          int data = doc["data"];

          Serial.printf("%s,%s,%d\n", device, port, data);
        } else {
          // Check for RGB keys (touch instrument)
          if (!doc["r"].isNull()) {
            Serial.printf("%s,r,%d\n", device, (int)doc["r"]);
          }

          if (!doc["g"].isNull()) {
            Serial.printf("%s,g,%d\n", device, (int)doc["g"]);
          }

          if (!doc["b"].isNull()) {
            Serial.printf("%s,b,%d\n", device, (int)doc["b"]);
          }
        }
      }
    }
  }

  // Handle Serial -> ESP Now
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      // Expected format: device,port,command,value
      int firstComma = line.indexOf(',');
      int secondComma = line.indexOf(',', firstComma + 1);
      int thirdComma = line.indexOf(',', secondComma + 1);

      if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
        String device = line.substring(0, firstComma);
        String port = line.substring(firstComma + 1, secondComma);
        String command = line.substring(secondComma + 1, thirdComma);
        String valueStr = line.substring(thirdComma + 1);

        JsonDocument doc;
        doc["device"] = device;
        doc["port"] = port;
        doc["command"] = command;
        doc["data"] = valueStr.toInt();

        char buffer[256];
        serializeJson(doc, buffer);

        // Check if we know the device's MAC address
        if (discoveredDevices.count(device)) {
          uint8_t* targetMac = discoveredDevices[device].data();

          // Ensure the device is added as a peer
          if (!esp_now_is_peer_exist(targetMac)) {
            esp_now_peer_info_t peerInfo = {};
            memcpy(peerInfo.peer_addr, targetMac, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;
            esp_now_add_peer(&peerInfo);
          }

          esp_now_send(targetMac, (uint8_t *)buffer, strlen(buffer) + 1);
        }

        // Flash LED for activity
        digitalWrite(LED_BUILTIN, HIGH); // Turn OFF (active low)
        ledFlashTime = millis();
      }
    }
  }
}
