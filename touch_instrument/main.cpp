#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "touch";

bool pingReceived = false;

const int touchPins[] = {D1, D2, D3};
const int sensitivityThreshold = 50; // Minimum change to trigger a send
uint32_t touchMinima[] = {39000, 45100, 46100};

// State tracking
uint32_t lastValues[] = {0, 0, 0};

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
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
  doc["deviceType"] = "sensor";
  doc["device"] = deviceName;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_now_send(relayAddress, (uint8_t *) buffer, strlen(buffer) + 1);
}

void sendPong() {
  JsonDocument doc;
  doc["command"] = "pong";
  doc["deviceType"] = "sensor";
  doc["device"] = deviceName;

  char buffer[128];
  serializeJson(doc, buffer);

  pingReceived = false;
  esp_now_send(relayAddress, (uint8_t *) buffer, strlen(buffer) + 1);
}

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

void sendEvent(String port, int value) {
  JsonDocument doc;

  doc["device"] = deviceName;
  doc["port"] = port;
  doc["data"] = value;

  char buffer[128];
  serializeJson(doc, buffer);

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

  // Establish touch minima
  for (int i=0; i<3; i++) {
    uint32_t baseline = touchRead(touchPins[i]);
    touchMinima[i] = baseline;
  }

  Serial.println("XIAO ESP32-S3 Touch Sender Ready");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (pingReceived) {
    Serial.println("Processing ping...");
    sendPong();
  }

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
      sendEvent("r", newR);
      sendEvent("g", newG);
      sendEvent("b", newB);
    }
  }

  delay(100);
}
