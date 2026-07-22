#include <esp_now.h>
#include <WiFi.h>
#include "Protocol.h"
#include <Bounce2.h>

#define DEBOUNCE_INTERVAL 10

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "maracas1";

bool pingReceived = false;

const int port = D3;
Bounce debouncer = Bounce();

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

void sendEvent() {
  DataPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DATA;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.port, "D3", sizeof(packet.port) - 1);
  packet.value = 1;

  esp_err_t result = esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));
  Serial.println("Sent event");

  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

void setup() {
  Serial.begin(115200);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(port, INPUT_PULLDOWN);
  debouncer.attach(port, INPUT_PULLDOWN);
  debouncer.interval(DEBOUNCE_INTERVAL); // 25ms debounce interval
  pinMode(LED_BUILTIN, OUTPUT);

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

  digitalWrite(LED_BUILTIN, LOW); // Turn on LED (active-low)
}

void loop() {
  if (pingReceived) {
    Serial.println("Processing ping...");
    sendPong();
  }

  debouncer.update();

  // Check for state change (rising edge)
  if (debouncer.changed()) {
    sendEvent();
  }

  delay(1); // Small delay to prevent tight loop, though debouncer handles timing
}
