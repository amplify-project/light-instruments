#include "LightInstrument.h"

LightInstrument* LightInstrument::instance = nullptr;

LightInstrument::LightInstrument(const char* deviceType) : deviceType(deviceType) {
  instance = this;
}

bool LightInstrument::begin(int resetPin) {
  Serial.begin(115200);

  if (resetPin != -1) {
    handleMemoryReset(resetPin);
  }

  if (!initDeviceConfig()) {
    Serial.println("Device name not set. Please set it using 'name=YOUR_NAME'. WiFi channel can be set using 'channel=CHANNEL_NUM'");
  } else {
    Serial.print("Device name: ");
    Serial.println(deviceName);
    Serial.print("WiFi Channel: ");
    Serial.println(wifiChannel);
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return false;
  }

  esp_now_register_recv_cb(onDataRecvStatic);
  return true;
}

void LightInstrument::signalBootStart() {
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
}

void LightInstrument::signalDeviceReady() {
  digitalWrite(LED_BUILTIN, LOW);
}

void LightInstrument::update() {
  if (pingReceived) {
    sendPong();
  }
}

void LightInstrument::onDataRecvStatic(const uint8_t* mac, const uint8_t* incomingData, int len) {
  if (instance) {
    instance->onDataRecv(mac, incomingData, len);
  }
}

void LightInstrument::onDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  if (len < (int)sizeof(ProtocolHeader)) {
    return;
  }

  ProtocolHeader* header = (ProtocolHeader*)incomingData;

  if (header->type == MSG_DISCOVERY) {
    if (!relayFound) {
      if (!addPeer(mac)) {
        return;
      }

      sendDiscoveryResponse();
    }
  } else if (header->type == MSG_PING) {
    pingReceived = true;

    if (!relayFound) {
      addPeer(mac);
    }
  }
}

bool LightInstrument::addPeer(const uint8_t* mac) {
  memcpy(relayAddress, mac, 6);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, relayAddress, 6);
  peerInfo.channel = wifiChannel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return false;
  }

  relayFound = true;
  Serial.println("Relay found and peer added");

  return true;
}

void LightInstrument::sendDiscoveryResponse() {
  if (!relayFound) {
    return;
  }

  DiscoveryResponsePacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DISCOVERY_RESPONSE;

  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.deviceType, deviceType, sizeof(packet.deviceType) - 1);

  esp_err_t result = esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));

  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

void LightInstrument::sendPong() {
  if (!relayFound) {
    return;
  }

  PongPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_PONG;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.deviceType, deviceType, sizeof(packet.deviceType) - 1);

  pingReceived = false;
  esp_err_t result = esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));

  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

void LightInstrument::sendEvent(const char* port, int32_t value) {
  if (!relayFound) {
    return;
  }

  DataPacket packet;
  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_DATA;
  strncpy(packet.deviceName, deviceName.c_str(), sizeof(packet.deviceName) - 1);
  strncpy(packet.port, port, sizeof(packet.port) - 1);
  packet.value = value;

  esp_err_t result = esp_now_send(relayAddress, (uint8_t *)&packet, sizeof(packet));

  if (result != ESP_OK) {
    Serial.println("Error sending the data");
  }
}

void LightInstrument::handleMemoryReset(int resetPin) {
  pinMode(resetPin, INPUT_PULLUP);
  delay(10);

  if (digitalRead(resetPin) == LOW) {
    Serial.println("Performing memory reset...");

    Preferences prefs;
    prefs.begin("system", false);
    prefs.clear();
    prefs.end();

    delay(1000);
    ESP.restart();
  }
}

bool LightInstrument::initDeviceConfig() {
  Preferences prefs;
  prefs.begin("system", true);

  deviceName = prefs.getString("name", "");
  wifiChannel = prefs.getInt("channel", 0);

  prefs.end();

  return (deviceName != "");
}

void LightInstrument::saveDeviceName(String name) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putString("name", name);
  prefs.end();

  deviceName = name;
}

void LightInstrument::saveWifiChannel(int channel) {
  Preferences prefs;
  prefs.begin("system", false);
  prefs.putInt("channel", channel);
  prefs.end();

  wifiChannel = channel;
}

bool LightInstrument::listenForDeviceConfig() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("name=")) {
      String newName = input.substring(5);

      if (newName.length() > 0) {
        saveDeviceName(newName);
        Serial.println("OK");

        return true;
      }

      Serial.println("ERR");
      return false;
    } else if (input.startsWith("channel=")) {
      int newChannel = input.substring(8).toInt();

      if (newChannel >= 0) {
        saveWifiChannel(newChannel);
        Serial.println("OK");

        return true;
      }

      Serial.println("ERR");
      return false;
    } else if (input.startsWith("getsettings")) {
      Serial.printf("type=sensor,name=%s,channel=%d\n", deviceName, wifiChannel);
    }
  }

  return false;
}
