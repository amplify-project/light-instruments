#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>

#ifdef USE_MMA8451
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#endif

#ifdef USE_MPU6050
#include "Accelerometer.h"
#endif

#include "Protocol.h"
#include "Accelerometer.h"

uint8_t relayAddress[6];
bool relayFound = false;
esp_now_peer_info_t peerInfo;
String deviceName = "";

#ifdef USE_MMA8451
Adafruit_MMA8451 mma = Adafruit_MMA8451();
#endif

float lastX = 0, lastY = 0, lastZ = 0;
bool peakSearchX = false, peakSearchY = false, peakSearchZ = false;
const float threshold = 30.0; // Acceleration threshold for shake detection
unsigned long lastEventTime = 0;
const unsigned long cooldown = 100; // ms between events to avoid double triggering

bool pingReceived = false;

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

void sendEvent() {
  DataPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DATA;

  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.port, "accel", sizeof(packet.port) - 1);
  packet.value = 1;

  esp_err_t result = esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));

  if (result != ESP_OK) {
    Serial.println("Error sending event");
  } else {
    Serial.println("Event sent");
  }
}

void setup() {
  Serial.begin(115200);

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

  #ifdef USE_MMA8451
  if (!mma.begin()) {
    Serial.println("Couldnt start MMA8451");
    while (1);
  }

  mma.setRange(MMA8451_RANGE_4_G);
  #endif

  #ifdef USE_MPU6050
  initAccelerometer();
  #endif

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

  #ifdef USE_MMA8451
  sensors_event_t event;
  mma.getEvent(&event);

  float x = abs(event.acceleration.x);
  float y = abs(event.acceleration.y);
  float z = abs(event.acceleration.z);
  #endif

  #ifdef USE_MPU6050
  AccelerationReading reading;
  readAccelerationValues(&reading);

  float x = reading.x;
  float y = reading.y;
  float z = reading.z;
  #endif

  bool triggered = false;
  unsigned long now = millis();

  if (now - lastEventTime > cooldown) {
    // Check X axis for apex
    if (x > threshold) {
      if (x > lastX) {
        peakSearchX = true;
      } else if (peakSearchX) {
        triggered = true;
      }
    } else {
      peakSearchX = false;
    }

    // Check Y axis for apex
    if (y > threshold) {
      if (y > lastY) {
        peakSearchY = true;
      } else if (peakSearchY) {
        triggered = true;
      }
    } else {
      peakSearchY = false;
    }

    // Check Z axis for apex
    if (z > threshold) {
      if (z > lastZ) {
        peakSearchZ = true;
      } else if (peakSearchZ) {
        triggered = true;
      }
    } else {
      peakSearchZ = false;
    }

    if (triggered) {
      sendEvent();

      lastEventTime = now;
      peakSearchX = false;
      peakSearchY = false;
      peakSearchZ = false;
    }
  }

  lastX = x;
  lastY = y;
  lastZ = z;

  delay(1);
}
