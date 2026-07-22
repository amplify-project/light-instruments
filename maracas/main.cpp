#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
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

void sendEvent() {
  JsonDocument doc;
  doc["device"] = deviceName;
  doc["port"] = "D3";
  doc["data"] = 1;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_err_t result = esp_now_send(relayAddress, (uint8_t *) buffer, strlen(buffer) + 1);
  Serial.print("Sent: ");
  Serial.println(buffer);

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
