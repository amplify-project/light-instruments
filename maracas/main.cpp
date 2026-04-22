#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
esp_now_peer_info_t peerInfo;
String deviceName = "rattle";

const int port = D3;
int lastState = 0;

void sendEvent() {
  JsonDocument doc;
  doc["device"] = deviceName;
  doc["event"] = true;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) buffer, strlen(buffer) + 1);
  Serial.print("Sent: ");
  Serial.println(buffer);

  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(port, INPUT);

  // Initialise ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register Peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  int currentState = digitalRead(port);

  // Check for state change
  if (currentState != lastState) {
    // Only send if state is HIGH
    if (currentState == HIGH) {
      sendEvent();
    }

    lastState = currentState;
  }

  delay(10);
}
