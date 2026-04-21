#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// --- Configuration ---
uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
const int touchPins[] = {D1, D2, D3};
const int numInputs = 3;
const int sensitivityThreshold = 3; // Minimum change to trigger a send

// State tracking
int lastValues[] = {0, 0, 0};

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("XIAO ESP32-S3 Touch Sender Ready");
}

void loop() {
  for (int i = 0; i < numInputs; i++) {
    int currentValue = touchRead(touchPins[i]);

    // Send data if the change exceeds our threshold
    if (abs(currentValue - lastValues[i]) > sensitivityThreshold) {
      sendJsonData(i + 1, currentValue);
      lastValues[i] = currentValue;
    }
  }

  delay(30);
}

void sendJsonData(int id, int val) {
  StaticJsonDocument<128> doc;
  doc["id"] = id;
  doc["val"] = val;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_now_send(broadcastAddress, (uint8_t *) buffer, strlen(buffer) + 1);

  Serial.printf("ID: %d | Raw Value: %d\n", id, val);
}
