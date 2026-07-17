#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;

String deviceName = "receiver1";
String deviceType = "actuator";

bool pingReceived = false;

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
  esp_now_send(relayAddress, (uint8_t *) buffer, strlen(buffer) + 1);
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

}
