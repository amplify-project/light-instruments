#include <deque>
#include <mutex>

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#define NUM_LEDS_D0 30
#define DATA_PIN_D0 D0

struct Packet {
  uint8_t mac[6];
  JsonDocument doc;
};

CRGB ledsD0[NUM_LEDS_D0];

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;

String deviceName = "receiver1";
String deviceType = "actuator";

bool pingReceived = false;

std::deque<Packet> packetQueue;
const size_t MAX_QUEUE_SIZE = 20;
std::mutex queueMtx;

struct AnimationState {
    bool active = false;
    uint32_t startTime = 0;
    uint16_t attack = 0;  // ms
    uint16_t decay = 0;   // ms
    uint16_t sustain = 0; // ms
    uint16_t release = 0; // ms
    uint8_t targetBrightness = 255;
    CRGB color = CRGB::White;
};

AnimationState pulseAnim;

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
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
    } else {
      // Protect the queue with a mutex since this callback runs in a different task context
      std::lock_guard<std::mutex> lock(queueMtx);

      if (packetQueue.size() < MAX_QUEUE_SIZE) {
        // Create a packet structure to store data and MAC
        Packet p;
        memcpy(p.mac, mac, 6);
        p.doc = std::move(doc); // Use move to avoid copying the JsonDocument

        packetQueue.push_back(std::move(p));
      } else {
        Serial.println("Packet queue full, dropping packet");
      }
    }
  }
}

void sendDiscoveryResponse() {
  JsonDocument doc;
  doc["command"] = "discoveryResponse";
  doc["deviceType"] = deviceType;
  doc["device"] = deviceName;

  char buffer[128];
  serializeJson(doc, buffer);

  esp_now_send(relayAddress, (uint8_t *) buffer, strlen(buffer) + 1);
}

void sendPong() {
  JsonDocument doc;
  doc["command"] = "pong";
  doc["deviceType"] = deviceType;
  doc["device"] = deviceName;

  char buffer[128];
  serializeJson(doc, buffer);

  pingReceived = false;
  esp_now_send(relayAddress, (uint8_t *)buffer, strlen(buffer) + 1);
}

void processLightCommand(const JsonDocument& doc) {
    const char* port = doc["port"];

    if (doc["command"] == "setColor") {
      const char* value = doc["value"];

      if (value) {
        // If no port is specified, default to D0, or check if it matches D0
        if (port == nullptr || strcmp(port, "D0") == 0) {
          int r, g, b;

          if (sscanf(value, "%d,%d,%d", &r, &g, &b) == 3) {
            fill_solid(ledsD0, NUM_LEDS_D0, CRGB(r, g, b));
            FastLED.show();
          }
        }
      }
    } else if (doc["command"] == "pulse") {
      const char* value = doc["value"];
      if (value && (port == nullptr || strcmp(port, "D0") == 0)) {
        int r, g, b, a, d, s, re;
        if (sscanf(value, "%d,%d,%d,%d,%d,%d,%d", &r, &g, &b, &a, &d, &s, &re) == 7) {
          pulseAnim.color = CRGB(r, g, b);
          pulseAnim.attack = a;
          pulseAnim.decay = d;
          pulseAnim.sustain = s;
          pulseAnim.release = re;
          pulseAnim.startTime = millis();
          pulseAnim.active = true;
        }
      }
    } else if (doc["command"] == "setBrightness") {
      const char* value = doc["value"];

      if (value) {
        if (port == nullptr || strcmp(port, "D0") == 0) {
          FastLED.setBrightness(atoi(value));
          FastLED.show();
        }
      }
    }
}

void processIncomingPackets() {
  Packet currentPacket;
  bool hasPacket = false;

  {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (!packetQueue.empty()) {
      currentPacket = std::move(packetQueue.front()); // Move out of the queue
      packetQueue.pop_front(); // Efficient removal from deque
      hasPacket = true;
    }
  }

  if (!hasPacket) {
    return;
  }

  // Packet is a command packet if the key 'command' is set
  if (!currentPacket.doc["command"].isNull()) {
    processLightCommand(currentPacket.doc);
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

  FastLED.addLeds<WS2812B, DATA_PIN_D0, GRB>(ledsD0, NUM_LEDS_D0);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();

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

void updateAnimations() {
  if (!pulseAnim.active) return;

  uint32_t now = millis();
  uint32_t elapsed = now - pulseAnim.startTime;
  uint8_t brightness = 0;

  if (elapsed < pulseAnim.attack) {
    // Attack phase: linear ramp up
    brightness = map(elapsed, 0, pulseAnim.attack, 0, 255);
  } else if (elapsed < pulseAnim.attack + pulseAnim.decay) {
    // Decay phase: ramp down to sustain level (using 128 as sustain brightness for now, or we could add it to params)
    // Actually, usually sustain is a level, but the user asked for "sustain" which in this context often means duration
    // Let's assume sustain is a duration at peak brightness (255) for simplicity, or 
    // interpret the 4 params as durations.
    brightness = map(elapsed - pulseAnim.attack, 0, pulseAnim.decay, 255, 200);
  } else if (elapsed < pulseAnim.attack + pulseAnim.decay + pulseAnim.sustain) {
    // Sustain phase: hold
    brightness = 200;
  } else if (elapsed < pulseAnim.attack + pulseAnim.decay + pulseAnim.sustain + pulseAnim.release) {
    // Release phase: ramp down to 0
    brightness = map(elapsed - (pulseAnim.attack + pulseAnim.decay + pulseAnim.sustain), 0, pulseAnim.release, 200, 0);
  } else {
    pulseAnim.active = false;
    brightness = 0;
  }

  fill_solid(ledsD0, NUM_LEDS_D0, pulseAnim.color);
  FastLED.setBrightness(brightness);
  FastLED.show();
}

void loop() {
  if (pingReceived) {
    sendPong();
  }

  processIncomingPackets();
  updateAnimations();
}
