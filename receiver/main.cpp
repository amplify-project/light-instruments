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

std::map<String, std::array<uint8_t, 6>> discoveredDevices;
unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 10000; // 10 seconds

unsigned long lastPingTime = 0;
const unsigned long PING_INTERVAL = 10000;

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
  packetQueue.push_back(p);

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
      sendPing(device.second.data());
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
    discoveredDevices[device] = mac;
  } else if (doc["command"] == "pong") {
    const char* device = doc["device"];
    const char* deviceType = doc["deviceType"];

    // Inform the editor that a device has responded to a ping
    Serial.printf("MSG,pong,%s,%s\n", deviceType, device);
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
  } else {
    // Check for RGB keys (touch instrument)
    if (!doc["r"].isNull()) {
      Serial.printf("DATA,%s,r,%d\n", device, (int)doc["r"]);
    }

    if (!doc["g"].isNull()) {
      Serial.printf("DATA,%s,g,%d\n", device, (int)doc["g"]);
    }

    if (!doc["b"].isNull()) {
      Serial.printf("DATA,%s,b,%d\n", device, (int)doc["b"]);
    }
  }
}

/**
 * @brief Sends a command to an output device through ESP Now. If the device
 * with the given name is not known, nothing happens.
 *
 * @param device Name of the device to send the data to
 * @param port Port on the device that the data should be sent to
 * @param command Command to send
 * @param value Parameters for the command
 */
void sendDeviceCommand(const String& device, const String& port, const String& command, int value) {
  // If the device name is not known, do nothing
  if (discoveredDevices.count(device) == 0) {
    return;
  }

  // Build JSON data
  JsonDocument doc;
  doc["device"] = device;
  doc["port"] = port;
  doc["command"] = command;
  doc["data"] = value;

  // Serialise packet to string
  char buffer[256];
  serializeJson(doc, buffer);

  // Get destination MAC address
  uint8_t* mac = discoveredDevices[device].data();

  // Ensure the device is added as a peer
  addPeer(mac);

  // Send packet and trigger builtin LED
  esp_now_send(mac, (uint8_t *)buffer, strlen(buffer) + 1);
  triggerActivityIndicator();
}

/**
 * @brief Processes data received through the serial connection.
 */
void processSerialInput() {
  // Return if no data is available
  if (Serial.available() == 0) {
    return;
  }

  // Read until the next newline
  String line = Serial.readStringUntil('\n');
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
    int value = line.substring(thirdComma + 1).toInt();

    // Send command to device
    sendDeviceCommand(device, port, command, value);
  }
}

void setup() {
  Serial.begin(115200);
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
