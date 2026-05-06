#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
esp_now_peer_info_t peerInfo;
String deviceName = "touch";

const int touchPins[] = {D1, D2, D3};
const uint32_t touchMinima[] = {39000, 45100, 46100};
const int sensitivityThreshold = 3; // Minimum change to trigger a send

// State tracking
uint32_t lastValues[] = {0, 0, 0};

int processValue(int i, uint32_t val) {
  int adjustedVal = val - touchMinima[i];

  if (adjustedVal < 0) {
    adjustedVal = 0;
  }

  if (adjustedVal > 50000) {
    adjustedVal = 50000;
  }

  return map(adjustedVal, 0, 50000, 0, 4096);
}

void sendJsonData(int r, int g, int b) {
  JsonDocument doc;

  doc["device"] = deviceName;
  doc["r"] = r;
  doc["g"] = g;
  doc["b"] = b;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_now_send(broadcastAddress, (uint8_t *) buffer, strlen(buffer) + 1);
  Serial.printf("r: %04d g: %04d b: %04d\n", r, g, b);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("XIAO ESP32-S3 Touch Sender Ready");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  bool sendData = false;

  uint32_t r = touchRead(touchPins[0]);
  if ((r - lastValues[0]) > sensitivityThreshold) {
    sendData = true;
    lastValues[0] = r;
  }

  uint32_t g = touchRead(touchPins[1]);
  if ((g - lastValues[1]) > sensitivityThreshold) {
    sendData = true;
    lastValues[1] = g;
  }

  uint32_t b = touchRead(touchPins[2]);
  if ((b - lastValues[2]) > sensitivityThreshold) {
    sendData = true;
    lastValues[2] = b;
  }

  if (sendData) {
    int newR = processValue(0, r);
    int newG = processValue(1, g);
    int newB = processValue(2, b);

    if (newR + newG + newB > 0) {
      sendJsonData(newR, newG, newB);
    }
  }

  delay(100);
}
