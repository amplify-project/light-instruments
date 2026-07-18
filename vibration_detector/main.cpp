#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "percussion_small";

bool pingReceived = false;

const int analogPin = A1;
const int threshold = 5; // Ignore minor voltage jitter

// State tracking
float smoothedValue = 0;
const float alpha = 0.6; // Smoothing factor (0.0 to 1.0). Lower = more smoothing, slower response.
int lastValue = -1;

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

void sendEvent(int val) {
  JsonDocument doc;
  doc["device"] = deviceName;
  doc["port"] = "A1";
  doc["data"] = val;

  char buffer[128];
  serializeJson(doc, buffer);
  esp_now_send(relayAddress, (uint8_t *)buffer, strlen(buffer) + 1);
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
