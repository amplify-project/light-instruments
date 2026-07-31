#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include "Protocol.h"

#define DEVICE_RESET D7

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "";

bool pingReceived = false;

const int analogPin = A1;
const int threshold = 5; // Ignore minor voltage jitter

// State tracking
float smoothedValue = 0;
const float alpha = 0.6; // Smoothing factor (0.0 to 1.0). Lower = more smoothing, slower response.
int lastValue = -1;

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

void sendEvent(int val) {
  DataPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DATA;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.port, "A1", sizeof(packet.port) - 1);
  packet.value = val;

  esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
}

void handleMemoryReset() {
  pinMode(DEVICE_RESET, INPUT_PULLUP);

  if (digitalRead(DEVICE_RESET) == LOW) {
    Serial.println("Performing memory reset...");
    Preferences prefs;

    prefs.begin("system", false);
    prefs.clear();
    prefs.end();
  }
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
  handleMemoryReset();

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

  pinMode(A1, INPUT);
  analogReadResolution(12);

  // Initialise ESP-NOW
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

  // Initialize smoothed value with current reading
  smoothedValue = analogRead(analogPin);

  Serial.println("XIAO ESP32-S3 vibration detector ready");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (pingReceived) {
    sendPong();
  }

  int rawValue = analogRead(analogPin);

  // Apply Exponential Moving Average (EMA) smoothing
  smoothedValue = (alpha * rawValue) + ((1.0 - alpha) * smoothedValue);
  int currentValue = (int)(smoothedValue + 0.5); // Round to nearest int
  currentValue = constrain(currentValue, 0, 1023); // Clamp value between 0 and 1023

  // Only send if the value has changed significantly
  if (abs(currentValue - lastValue) > threshold) {
    sendEvent(currentValue);
    lastValue = currentValue;
  }

  Serial.println(currentValue);
  delay(20);
}
