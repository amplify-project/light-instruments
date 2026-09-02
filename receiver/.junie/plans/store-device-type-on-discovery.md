---
sessionId: session-260722-154738-2wu9
---

# Requirements

### Overview & Goals
The goal of this task is to ensure that the `deviceType` of discovered devices is stored in memory alongside their name and MAC address. Currently, only the name and MAC address are stored in the `discoveredDevices` map.

### Scope
- **In Scope**:
    - Modification of the data structure used to store discovered devices.
    - Updating the discovery logic to include `deviceType` in the stored information.
    - Updating existing code that interacts with the `discoveredDevices` map to reflect the data structure changes.
- **Out of Scope**:
    - Changing the ESP Now communication protocol.
    - Modifying the serial output format (which already includes `deviceType`).

# Technical Design

### Current Implementation
- `discoveredDevices` is a `std::map<String, std::array<uint8_t, 6>>` where the key is the device name and the value is the MAC address.
- `processCommand` handles `discoveryResponse` packets, extracts `deviceType`, but only stores the MAC address in the map.

### Proposed Changes
1. **Define `DeviceInfo` struct**: Introduce a struct to group device-related information.
   ```cpp
   struct DeviceInfo {
     std::array<uint8_t, 6> mac;
     String type;
   };
   ```
2. **Update `discoveredDevices` map**: Change the map value type to `DeviceInfo`.
   ```cpp
   std::map<String, DeviceInfo> discoveredDevices;
   ```
3. **Update Discovery Logic**: In `processCommand`, populate the `DeviceInfo` struct.
   ```cpp
   discoveredDevices[device] = { mac, deviceType };
   ```
4. **Update Usage Sites**:
   - `handlePingInterval`: Access `device.second.mac.data()` instead of `device.second.data()`.
   - `sendDeviceCommand`: Access `discoveredDevices[device].mac.data()` instead of `discoveredDevices[device].data()`.

### File Structure
- `main.cpp`: All changes will be contained in this file.

# Delivery Steps

### ✓ Step 1: Update device storage data structure
Define the `DeviceInfo` struct and update the `discoveredDevices` map definition in `main.cpp`.
- Create a new `struct DeviceInfo` to hold the MAC address and the device type string.
- Change the `discoveredDevices` map from `std::map<String, std::array<uint8_t, 6>>` to `std::map<String, DeviceInfo>`.

### ✓ Step 2: Implement deviceType storage and update usage sites
Update the `processCommand` and `sendDeviceCommand` functions to handle the new `DeviceInfo` struct.
- In `processCommand`, store both the MAC address and the `deviceType` in the `discoveredDevices` map when a `discoveryResponse` is received.
- In `sendDeviceCommand`, update the MAC address retrieval to use the `mac` field from the `DeviceInfo` struct.
- In `handlePingInterval`, update the iteration to access the `mac` field from the `DeviceInfo` struct.