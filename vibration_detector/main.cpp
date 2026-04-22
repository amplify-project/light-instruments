#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
String deviceName = "percussion_big";

const int analogPin = A1;
const int threshold = 5; // Ignore minor voltage jitter

// State tracking
int lastValue = -1;

void sendJsonData(int val) {
  StaticJsonDocument<128> doc;
  doc["device"] = deviceName;
  doc["val"] = val;

  char buffer[128];
  serializeJson(doc, buffer);
  esp_now_send(broadcastAddress, (uint8_t *)buffer, strlen(buffer) + 1);

  Serial.printf("Value: %d\n", val);
}

void setup() {
  Serial.begin(115200);

  pinMode(A1, INPUT);
  analogReadResolution(12);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("XIAO ESP32-S3 vibration detector ready");
}

void loop() {
  int currentValue = analogRead(analogPin);

  // Only send if the value has changed significantly
  if (abs(currentValue - lastValue) > threshold) {
    sendJsonData(currentValue);
    lastValue = currentValue;
  }

  delay(20);
}
