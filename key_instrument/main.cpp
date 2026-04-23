#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define BTN_PRESSED 1
#define BTN_RELEASED 0

uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
esp_now_peer_info_t peerInfo;
String deviceName = "keys";

const int buttonPins[] = {D1, D2, D3};
const int numButtons = 3;

// State tracking
int lastStates[] = {HIGH, HIGH, HIGH};

void sendButtonEvent(int id, int action) {
  JsonDocument doc;
  doc["device"] = deviceName;
  doc["port"] = id;
  doc["data"] = action;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) buffer, strlen(buffer) + 1);

  Serial.printf("Sending button ID: %d, event: %d\n", id, action);

  if (result == ESP_OK) {
    Serial.print("Sent: ");
    Serial.println(buffer);
  } else {
    Serial.println("Error sending the data");
  }
}

void setup() {
  Serial.begin(115200);

  // Initialise Pins
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastStates[i] = digitalRead(buttonPins[i]);
  }

  // Initialise ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

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

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  for (int i = 0; i < numButtons; i++) {
    int currentState = digitalRead(buttonPins[i]);

    // Check for state change
    if (currentState != lastStates[i]) {
      sendButtonEvent(i + 1, currentState == LOW ? BTN_RELEASED : BTN_PRESSED);
      lastStates[i] = currentState;
    }
  }
  delay(10);
}
