#include <esp_now.h>
#include <WiFi.h>
#include "Protocol.h"

#define BTN_PRESSED 1
#define BTN_RELEASED 0

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "keys";

bool pingReceived = false;

const int buttonPins[] = {D1, D2, D3};
const int numButtons = 3;

// State tracking
int lastStates[] = {HIGH, HIGH, HIGH};

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

void sendEvent(int id, int action) {
  DataPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DATA;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  snprintf(packet.port, sizeof(packet.port), "%d", id);
  packet.value = action;

  esp_err_t result = esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));

  Serial.printf("Sending button ID: %d, event: %d\n", id, action);

  if (result == ESP_OK) {
    Serial.println("Sent successfully");
  } else {
    Serial.println("Error sending the data");
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

  // Initialise Pins
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastStates[i] = digitalRead(buttonPins[i]);
  }

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
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (pingReceived) {
    Serial.println("Processing ping...");
    sendPong();
  }

  for (int i = 0; i < numButtons; i++) {
    int currentState = digitalRead(buttonPins[i]);

    // Check for state change
    if (currentState != lastStates[i]) {
      sendEvent(i + 1, currentState == LOW ? BTN_RELEASED : BTN_PRESSED);
      lastStates[i] = currentState;
    }
  }
  delay(10);
}
