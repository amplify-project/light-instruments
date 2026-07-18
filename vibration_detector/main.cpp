#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>

uint8_t broadcastAddress[] = {0x2C, 0xF4, 0x32, 0x4E, 0xB2, 0xBE};
esp_now_peer_info_t peerInfo;
String deviceName = "percussion_small";

const int analogPin = A1;
const int threshold = 5; // Ignore minor voltage jitter

// State tracking
float smoothedValue = 0;
const float alpha = 0.15; // Smoothing factor (0.0 to 1.0). Lower = more smoothing, slower response.
int lastValue = -1;

void sendJsonData(int val) {
  JsonDocument doc;
  doc["device"] = deviceName;
  doc["port"] = "A1";
  doc["data"] = val;

  char buffer[128];
  serializeJson(doc, buffer);
  esp_now_send(broadcastAddress, (uint8_t *)buffer, strlen(buffer) + 1);

  Serial.printf("Value: %d\n", val);
}

void setup() {
  Serial.begin(115200);

  pinMode(A1, INPUT);
  analogReadResolution(12);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("XIAO ESP32-S3 vibration detector ready");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize smoothed value with current reading
  smoothedValue = analogRead(analogPin);
}

void loop() {
  int rawValue = analogRead(analogPin);

  // Apply Exponential Moving Average (EMA) smoothing
  smoothedValue = (alpha * rawValue) + ((1.0 - alpha) * smoothedValue);
  int currentValue = (int)(smoothedValue + 0.5); // Round to nearest int
  currentValue = constrain(currentValue, 0, 1023); // Clamp value between 0 and 1023

  // Only send if the value has changed significantly
  if (abs(currentValue - lastValue) > threshold) {
    sendJsonData(currentValue);
    lastValue = currentValue;
  }

  delay(20);
}
