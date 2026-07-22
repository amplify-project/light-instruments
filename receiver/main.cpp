#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_now.h>
#include <vector>
#include <mutex>
#include <map>
#include <array>

struct Packet {
  uint8_t mac[6];
  String data;
};

std::vector<Packet> packetQueue;
std::mutex queueMtx;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct DeviceInfo {
  std::array<uint8_t, 6> mac;
  String type;
};

std::map<String, DeviceInfo> discoveredDevices;
unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 10000; // 10 seconds

unsigned long lastPingTime = 0;
const unsigned long PING_INTERVAL = 10000;

const size_t MAX_QUEUE_SIZE = 100;
String serialBuffer = "";

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
  p.data = String((const char*)data, len);

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
  JsonDocument doc;
  doc["command"] = "discovery";

  char buffer[128];
  serializeJson(doc, buffer);
  esp_now_send(broadcastAddress, (uint8_t *)buffer, strlen(buffer) + 1);

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
  JsonDocument doc;
  doc["command"] = "ping";

  char buffer[128];
  serializeJson(doc, buffer);

  // Ensure the device is added as a peer
  addPeer(destination);
  esp_now_send(destination, (uint8_t *)buffer, strlen(buffer) + 1);

  triggerActivityIndicator();
}

/**
 * @brief Sends out ping packets to all discovered devices in periodic intervals determined
 * by PING_INTERVAL.
 */
void handlePingInterval() {
  if (millis() - lastPingTime > PING_INTERVAL) {
    lastPingTime = millis();

    for (auto const& device : discoveredDevices) {
      sendPing(device.second.mac.data());
    }
  }
}

/**
 * @brief Processes a received command package like device discovery requests
 * and responses.
 *
 * @param packet Received data packet
 * @param doc Parsed JSON data representing the received command
 */
void processCommand(const Packet& packet, const JsonDocument& doc) {
  if (doc["command"] == "discoveryResponse") {
    // Extract device name, type and MAC address from packet
    const char* device = doc["device"];
    const char* deviceType = doc["deviceType"];
    std::array<uint8_t, 6> mac;
    memcpy(mac.data(), packet.mac, 6);

    // Inform the editor that a device has been discovered
    Serial.printf("MSG,discovery,%s,%s\n", deviceType, device);

    // Store device name and MAC address in list of discovered devices
    discoveredDevices[device] = { mac, deviceType };
  } else if (doc["command"] == "pong") {
    const char* device = doc["device"];
    const char* deviceType = doc["deviceType"];

    // Inform the editor that a device has responded to a ping
    Serial.printf("MSG,pong,%s,%s\n", deviceType, device);
    // Send current packet queue length to editor
    Serial.printf("MSG,queuelen,%d\n", packetQueue.size());
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

  // Deserialise packet data
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, currentPacket.data);

  if (error) {
    return;
  }

  const char* device = doc["device"];

  // Return if the packet does not contain a device name
  if (!device) {
    return;
  }

  // Packet is a command packet if the key 'command' is set
  if (!doc["command"].isNull()) {
    // Process the command packet and return
    processCommand(currentPacket, doc);
    return;
  }

  // If the packet data contains the keys 'port' and 'data', extract the values
  // and print it to the serial connection
  if (!doc["port"].isNull() && !doc["data"].isNull()) {
    Serial.printf("DATA,%s,%s,%d\n", device, (const char*)doc["port"], (int)doc["data"]);
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
void sendToDevice(const String& device, const uint8_t* mac, const String& port, const String& command, const String& value) {
  // Build JSON data
  JsonDocument doc;
  doc["device"] = device;
  doc["port"] = port;
  doc["command"] = command;
  doc["data"] = value;

  // Serialise packet to string
  char buffer[256];
  serializeJson(doc, buffer);

  // Ensure the device is added as a peer
  addPeer(mac);

  // Send packet and trigger builtin LED
  esp_now_send(mac, (uint8_t *)buffer, strlen(buffer) + 1);
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
void sendDeviceCommand(const String& device, const String& port, const String& command, const String& value) {
  if (device.length() == 0) {
    // Forward to all actuators
    for (auto const& d : discoveredDevices) {
      if (d.second.type == "actuator") {
        sendToDevice(d.first, d.second.mac.data(), port, command, value);
      }
    }
  } else {
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
void handleSerialCommand(String line) {
  line.trim();

  // Return if the line is empty
  if (line.length() == 0) {
    return;
  }

  // Expected format: device,port,command,value
  int firstComma = line.indexOf(',');
  int secondComma = line.indexOf(',', firstComma + 1);
  int thirdComma = line.indexOf(',', secondComma + 1);

  // Make sure received data has the right format
  if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
    // Extract command parameters
    String device = line.substring(0, firstComma);
    String port = line.substring(firstComma + 1, secondComma);
    String command = line.substring(secondComma + 1, thirdComma);
    String value = line.substring(thirdComma + 1);

    // Send command to device
    sendDeviceCommand(device, port, command, value);
  }
}

/**
 * @brief Processes data received through the serial connection in a non-blocking way.
 */
void processSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      handleSerialCommand(serialBuffer);
      serialBuffer = "";
    } else if (c != '\r') {
      serialBuffer += c;
    }
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
}

void loop() {
  updateActivityIndicator();
  handleDiscoveryInterval();
  processIncomingPackets();
  processSerialInput();
  handlePingInterval();
}
