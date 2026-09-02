# Requirements

### Overview & Goals
Modify `sendDeviceCommand` so that if the `device` parameter is empty, the command is forwarded to all discovered devices of type 'actuator'.

### Scope
- **In Scope**:
    - Refactoring `sendDeviceCommand` to avoid duplication.
    - Implementing the logic to iterate and filter by `deviceType`.
- **Out of Scope**:
    - Changing the discovery mechanism.
    - Modifying serial input parsing (unless necessary to support empty device names, but it seems already supported).

# Technical Design

### Proposed Changes
1. **Extract `sendToDevice` helper**:
   Create a helper function that takes the MAC address and the command parameters to handle the actual ESP Now sending and LED triggering.
2. **Update `sendDeviceCommand`**:
   - Check if `device` is empty.
   - If empty, iterate through `discoveredDevices` and call `sendToDevice` for each device with `type == "actuator"`.
   - If not empty, proceed as before (check if known, then send).

# Delivery Steps

### ✓ Step 1: Implement command forwarding to actuators
Refactor `sendDeviceCommand` in `main.cpp` and implement the broadcast-to-actuators logic.
- Create `sendToDevice` helper function.
- Update `sendDeviceCommand` to handle empty device strings.
