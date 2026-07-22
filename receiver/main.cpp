#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <vector>
#include <mutex>
#include <map>
#include <array>

#include "Protocol.h"

struct Packet {
  uint8_t mac[6];
  uint8_t data[250];
  int len;
};

std::vector<Packet> packetQueue;
std::mutex queueMtx;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct DeviceInfo {
  std::array<uint8_t, 6> mac;
  String type;
};

std::map<String, DeviceInfo> discoveredDevices;
std::mutex devicesMtx;
unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 10000; // 10 seconds

unsigned long lastPingTime = 0;
const unsigned long PING_INTERVAL = 10000;

const size_t MAX_QUEUE_SIZE = 100;
char serialBuffer[256];
size_t serialBufferLen = 0;

volatile unsigned long ledFlashTime = 0;
const int FLASH_DURATION = 50;

/**
 * @brief Adds a new peer with the given MAC address to the peer list of the
 * ESP Now library.
 *
 * @param mac MAC address of the peer
 */
void addPeer(const uint8_t *mac) {
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_add_peer(&peerInfo);
  }
}

/**
 * @brief Toggles the builtin LED to indicate network activity.
 */
void triggerActivityIndicator() {
  digitalWrite(LED_BUILTIN, HIGH); // Turn OFF (active low)
  ledFlashTime = millis();
}

/**
 * @brief Callback invoked whenever a packet is received via ESP Now. Puts
 * received packets into a queue.
 *
 * @param macAddr MAC address the data came from
 * @param data The data that was received
 * @param len The length of the received data in bytes
 */
void onReceive(const uint8_t *macAddr, const uint8_t *data, int len) {
  // Create a packet structure to store data and MAC
  Packet p;
  memcpy(p.mac, macAddr, 6);
  p.len = (len > 250) ? 250 : len;
  memcpy(p.data, data, p.len);

  // Protect the queue with a mutex since this callback runs in a different task context
  std::lock_guard<std::mutex> lock(queueMtx);

  // Prevent unbounded growth which can cause crashes
  if (packetQueue.size() < MAX_QUEUE_SIZE) {
    packetQueue.push_back(p);
  }

  // Flash LED for activity
  triggerActivityIndicator();
}

/**
 * @brief Turns the builtin LED back on if FLASH_DURATION has elapsed.
 */
void updateActivityIndicator() {
  if (ledFlashTime > 0 && millis() - ledFlashTime > FLASH_DURATION) {
    digitalWrite(LED_BUILTIN, LOW); // Turn back ON (Ready state)
    ledFlashTime = 0;
  }
}

/**
 * @brief Send a device discovery packet to the broadcast address.
 */
void sendDiscovery() {
  DiscoveryPacket packet;
  esp_now_send(broadcastAddress, (uint8_t *)&packet, sizeof(packet));

  triggerActivityIndicator();
}

/**
 * @brief Sends out device discovery packets in periodic intervals determined
 * by DISCOVERY_INTERVAL.
 */
void handleDiscoveryInterval() {
  if (millis() - lastDiscoveryTime > DISCOVERY_INTERVAL) {
    lastDiscoveryTime = millis();
    sendDiscovery();
  }
}

/**
 * @brief Send a ping packet to the given address.
 *
 * @param destination MAC address of the target device
 */
void sendPing(const uint8_t* destination) {
  PingPacket packet;

  // Ensure the device is added as a peer
  addPeer(destination);
  esp_now_send(destination, (uint8_t *)&packet, sizeof(packet));

  triggerActivityIndicator();
}

/**
 * @brief Sends out ping packets to all discovered devices in periodic intervals determined
 * by PING_INTERVAL.
 */
void handlePingInterval() {
  if (millis() - lastPingTime > PING_INTERVAL) {
    lastPingTime = millis();

    std::lock_guard<std::mutex> lock(devicesMtx);
    for (auto const& device : discoveredDevices) {
      sendPing(device.second.mac.data());
    }
  }
}

/**
 * @brief Fetches packets from the packet queue and processes them. Command
 * packets by running the appropriate code and data packets are parsed and
 * forwarded through the serial port.
 */
void processIncomingPackets() {
  Packet currentPacket;
  bool hasPacket = false;

  {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (!packetQueue.empty()) {
      currentPacket = packetQueue.front();
      packetQueue.erase(packetQueue.begin());
      hasPacket = true;
    }
  }

  if (!hasPacket) {
    return;
  }

  if (currentPacket.len < (int)sizeof(ProtocolHeader)) {
    return;
  }

  ProtocolHeader* header = (ProtocolHeader*)currentPacket.data;

  switch (header->type) {
    case MSG_DISCOVERY_RESPONSE: {
      if (currentPacket.len < (int)sizeof(DiscoveryResponsePacket)) return;
      DiscoveryResponsePacket* p = (DiscoveryResponsePacket*)currentPacket.data;

      std::array<uint8_t, 6> mac;
      memcpy(mac.data(), currentPacket.mac, 6);

      Serial.printf("MSG,discovery,%s,%s\n", p->deviceType, p->deviceName);
      {
        std::lock_guard<std::mutex> lock(devicesMtx);
        discoveredDevices[p->deviceName] = { mac, p->deviceType };
      }
      break;
    }
    case MSG_PONG: {
      if (currentPacket.len < (int)sizeof(PongPacket)) return;
      PongPacket* p = (PongPacket*)currentPacket.data;

      Serial.printf("MSG,pong,%s,%s\n", p->deviceType, p->deviceName);
      Serial.printf("MSG,queuelen,%d\n", packetQueue.size());
      break;
    }
    case MSG_DATA: {
      if (currentPacket.len < (int)sizeof(DataPacket)) return;
      DataPacket* p = (DataPacket*)currentPacket.data;

      Serial.printf("DATA,%s,%s,%d\n", p->deviceName, p->port, p->value);
      break;
    }
    default:
      break;
  }
}

/**
 * @brief Sends a command to an output device through ESP Now.
 *
 * @param device Name of the device to send the data to
 * @param mac MAC address of the device
 * @param port Port on the device that the data should be sent to
 * @param command Command to send
 * @param value Parameters for the command
 */
void sendToDevice(const char* device, const uint8_t* mac, const char* port, const char* command, const char* value) {
  CommandPacket packet;

  memset(&packet, 0, sizeof(packet));
  packet.type = MSG_COMMAND;
  strncpy(packet.deviceName, device, sizeof(packet.deviceName) - 1);
  strncpy(packet.port, port, sizeof(packet.port) - 1);
  strncpy(packet.command, command, sizeof(packet.command) - 1);
  strncpy(packet.value, value, sizeof(packet.value) - 1);

  // Ensure the device is added as a peer
  addPeer(mac);

  // Send packet and trigger builtin LED
  esp_now_send(mac, (uint8_t *)&packet, sizeof(packet));
  triggerActivityIndicator();
}

/**
 * @brief Sends a command to an output device through ESP Now. If the device
 * with the given name is not known, nothing happens. If the device name is
 * empty, the command is forwarded to all known devices of type 'actuator'.
 *
 * @param device Name of the device to send the data to
 * @param port Port on the device that the data should be sent to
 * @param command Command to send
 * @param value Parameters for the command
 */
void sendDeviceCommand(const char* device, const char* port, const char* command, const char* value) {
  if (device == nullptr || strlen(device) == 0) {
    // Forward to all actuators
    std::lock_guard<std::mutex> lock(devicesMtx);
    for (auto const& d : discoveredDevices) {
      if (d.second.type == "actuator") {
        sendToDevice(d.first.c_str(), d.second.mac.data(), port, command, value);
      }
    }
  } else {
    std::lock_guard<std::mutex> lock(devicesMtx);
    // If the device name is not known, do nothing
    if (discoveredDevices.count(device) == 0) {
      return;
    }

    sendToDevice(device, discoveredDevices[device].mac.data(), port, command, value);
  }
}

/**
 * @brief Handles a single command line received via Serial.
 *
 * @param line The command line to process
 */
void handleSerialCommand(char* line) {
  // Expected format: device,port,command,value
  // We manually find the first three commas to allow the 'value' field to contain commas

  char* device = line;
  char* comma1 = strchr(device, ',');
  if (!comma1) return;
  *comma1 = '\0';
  char* port = comma1 + 1;

  char* comma2 = strchr(port, ',');
  if (!comma2) return;
  *comma2 = '\0';
  char* command = comma2 + 1;

  char* comma3 = strchr(command, ',');
  if (!comma3) return;
  *comma3 = '\0';
  char* value = comma3 + 1;

  sendDeviceCommand(device, port, command, value);
}

/**
 * @brief Processes data received through the serial connection in a non-blocking way.
 */
void processSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      serialBuffer[serialBufferLen] = '\0';
      handleSerialCommand(serialBuffer);
      serialBufferLen = 0;
    } else if (c != '\r' && serialBufferLen < sizeof(serialBuffer) - 1) {
      serialBuffer[serialBufferLen++] = c;
    }
  }
}

void serialTask(void *pvParameters) {
  for (;;) {
    processSerialInput();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void logicTask(void *pvParameters) {
  for (;;) {
    processIncomingPackets();
    handleDiscoveryInterval();
    handlePingInterval();
    updateActivityIndicator();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup() {
  Serial.begin(460800);
  delay(500); // Give the serial monitor time to connect

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    // Could not initialise ESP Now
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(onReceive));

  // Add broadcast peer for sending commands
  esp_now_peer_info_t peerInfo = {};
  memset(&peerInfo, 0, sizeof(peerInfo));

  // Build broadcast address
  for (int i = 0; i < 6; i++) {
    peerInfo.peer_addr[i] = 0xFF;
  }

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    // Failed to add broadcast peer
    return;
  }

  // Turn on builtin LED to indicate successful initialization
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Send initial discovery message
  sendDiscovery();
  lastDiscoveryTime = millis();

  lastPingTime = millis();

  xTaskCreatePinnedToCore(serialTask, "SerialTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(logicTask, "LogicTask", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // Tasks are running in background
  vTaskDelay(pdMS_TO_TICKS(1000));
}
