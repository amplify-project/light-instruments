#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include "Protocol.h"

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "";

bool pingReceived = false;

const int touchPins[] = {D1, D2, D3};
const int sensitivityThreshold = 10; // Minimum change in 0-1023 scale to trigger a send
uint32_t touchMinima[] = {0, 0, 0};
const uint32_t touchMaxDiff = 30000; // Expected max increase from baseline to reach 1023

// State tracking
int lastSentValues[] = {0, 0, 0};
float filteredValues[] = {0, 0, 0};
const float filterAlpha = 0.1f; // Smoothing factor (0.0 to 1.0), lower is smoother

void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len < (int)sizeof(ProtocolHeader)) return;
  ProtocolHeader* header = (ProtocolHeader*)incomingData;

  if (header->type == MSG_DISCOVERY) {
    if (!relayFound) {
      memcpy(relayAddress, mac, 6);
      relayFound = true;
    }
  } else if (header->type == MSG_PING) {
    pingReceived = true;
  }
}

void sendDiscoveryResponse() {
  DiscoveryResponsePacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DISCOVERY_RESPONSE;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.deviceType, "sensor", sizeof(packet.deviceType) - 1);

  esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
}

void sendPong() {
  PongPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_PONG;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.deviceType, "sensor", sizeof(packet.deviceType) - 1);

  pingReceived = false;
  esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
}

int processValue(int i, uint32_t val) {
  if (val < touchMinima[i]) {
    return 0;
  }

  uint32_t adjustedVal = val - touchMinima[i];

  if (adjustedVal > touchMaxDiff) {
    adjustedVal = touchMaxDiff;
  }

  return map(adjustedVal, 0, touchMaxDiff, 0, 1023);
}

void sendEvent(String port, int value) {
  DataPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DATA;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.port, port.c_str(), sizeof(packet.port) - 1);
  packet.value = value;

  esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
}

bool initDeviceName() {
  Preferences prefs;

  prefs.begin("system", true);
  deviceName = prefs.getString("name", "");
  prefs.end();

  return (deviceName != "");
}

void saveDeviceName(String name) {
  Preferences prefs;

  prefs.begin("system", false);
  prefs.putString("name", name);
  prefs.end();

  deviceName = name;
}

bool listenForDeviceName() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("name=")) {
      String newName = input.substring(5);

      if (newName.length() > 0) {
        saveDeviceName(newName);

        Serial.print("Device name updated and saved to flash: ");
        Serial.println(deviceName);

        return true;
      }
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);

  if (!initDeviceName()) {
    Serial.println("No persistent name found. Waiting for name=... command via Serial.");

    while (!listenForDeviceName()) {
      delay(100);
    }
  }

  Serial.print("Device Name: ");
  Serial.println(deviceName);

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

  // Establish touch minima with averaging for better consistency
  Serial.println("Calibrating touch sensors (do not touch)...");

  for (int i=0; i<3; i++) {
    uint64_t sum = 0;
    const int samples = 64;

    for (int j=0; j<samples; j++) {
      sum += touchRead(touchPins[i]);
      delay(5);
    }

    touchMinima[i] = (uint32_t)(sum / samples);
    filteredValues[i] = (float)touchMinima[i];

    Serial.printf("Pin D%d baseline: %u\n", i+1, touchMinima[i]);
  }

  Serial.println("XIAO ESP32-S3 Touch Sender Ready");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (pingReceived) {
    Serial.println("Processing ping...");
    sendPong();
  }

  bool changed = false;
  int currentProcessed[3];

  for (int i = 0; i < 3; i++) {
    uint32_t raw = touchRead(touchPins[i]);
    // Apply low-pass filter (EMA) to reduce noise from tin foil pads
    filteredValues[i] = (raw * filterAlpha) + (filteredValues[i] * (1.0f - filterAlpha));

    int processed = processValue(i, (uint32_t)filteredValues[i]);
    currentProcessed[i] = processed;

    // Check if the change is significant on the 0-1023 scale
    if (abs(processed - lastSentValues[i]) >= sensitivityThreshold) {
      changed = true;
    }
  }

  if (changed) {
    // Update last sent values and send to relay
    for (int i = 0; i < 3; i++) {
      lastSentValues[i] = currentProcessed[i];
    }

    sendEvent("r", currentProcessed[0]);
    sendEvent("g", currentProcessed[1]);
    sendEvent("b", currentProcessed[2]);
  }

  delay(20); // Faster loop for more responsive touch, smoothing handles the noise
}
