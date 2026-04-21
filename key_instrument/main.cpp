#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
String deviceName = "keys";

const int buttonPins[] = {D1, D2, D3};
const int numButtons = 3;

// State tracking
int lastStates[] = {HIGH, HIGH, HIGH};

void setup() {
  Serial.begin(115200);

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

  // Register Peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  for (int i = 0; i < numButtons; i++) {
    int currentState = digitalRead(buttonPins[i]);

    // Check for state change
    if (currentState != lastStates[i]) {
      sendButtonEvent(i + 1, currentState == LOW ? "pressed" : "released");
      lastStates[i] = currentState;
    }
  }
  delay(10);
}

void sendButtonEvent(int id, String action) {
  StaticJsonDocument<128> doc;
  doc["device"] = deviceName;
  doc["id"] = id;
  doc["event"] = action;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) buffer, strlen(buffer) + 1);

  if (result == ESP_OK) {
    Serial.print("Sent: ");
    Serial.println(buffer);
  } else {
    Serial.println("Error sending the data");
  }
}
