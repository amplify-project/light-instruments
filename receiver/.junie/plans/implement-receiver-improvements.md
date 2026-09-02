---
sessionId: session-260722-192429-uy78
---

# Requirements

### Overview & Goals
The goal is to implement the key improvements suggested in `IMPROVEMENTS.md` for the ESP32 receiver. These changes focus on making the system more robust for production use (e.g., live performances) by ensuring predictable memory usage, enabling scalability beyond 20 devices, and improving overall responsiveness through multi-core processing.

### Scope
- **In Scope**:
    - Memory optimization (Static buffers).
    - Scalability (LRU Peer Management).
    - Reliability (Watchdog, Packet ACKs).
    - Performance (FreeRTOS Tasking).
    - Monitoring (RSSI reporting).
- **Out of Scope**:
    - Changes to sensor/actuator firmware (Discovery logic, Sleep modes).
    - ESP-NOW Encryption (Optional/deferred).
    - NVS Configuration storage (Deferred).


# Technical Design

### Current Implementation
- **Memory**: Uses Arduino `String` for Serial input, which can lead to heap fragmentation.
- **ESP-NOW**: Adds peers indefinitely using `esp_now_add_peer` without checking the hardware limit of 20 peers.
- **Architecture**: Single-threaded `loop()` handling Serial I/O, packet processing, and timing-based pings/discovery.
- **Feedback**: No confirmation of whether transmitted commands reached the destination.

### Key Decisions
1. **LRU Strategy**: We will maintain a list of active peers. When a new peer is needed and the limit (20) is reached, the peer that hasn't been communicated with for the longest time will be removed.
2. **Task Allocation**: 
    - **Core 0**: Serial RX (Reading from desktop app). This core is less busy with system tasks.
    - **Core 1**: ESP-NOW and Serial TX. This ensures wireless processing takes priority.
3. **Static Allocation**: All buffers for Serial and Packet processing will be pre-allocated to avoid `malloc` at runtime.

### Proposed Changes

#### LRU Peer Management
```cpp
struct PeerInfo {
    uint8_t mac[6];
    uint32_t lastUsed;
};
std::vector<PeerInfo> activePeers;
const int MAX_PEERS = 19; // 20 minus 1 for broadcast
```

#### Multi-Core Tasking
- **SerialTask (Core 0)**: Reads from `Serial`, parses commands, and places them into a `commandQueue`.
- **MainTask (Core 1)**: Pulls from `commandQueue`, sends via ESP-NOW, and processes incoming ESP-NOW packets from the `packetQueue`.

#### RSSI & ACKs
- Update `onReceive` signature to match the latest ESP-IDF/Arduino version that provides `esp_now_recv_info_t`.
- Implement `onSend` callback to report `ESP_NOW_SEND_SUCCESS` or `ESP_NOW_SEND_FAIL`.

### File Structure
- `main.cpp`: Major refactoring to introduce FreeRTOS tasks and LRU logic.
- `Protocol.h`: (Optional) Add status message types if needed for ACKs.


# Testing

### Validation Approach
Verification will be performed by simulating multiple devices and monitoring the Serial output of the receiver.

### Key Scenarios
1. **Memory Stability**: Send a high volume of long Serial commands to verify that the static buffer doesn't overflow and memory usage remains constant.
2. **Peer Limit (LRU)**: Add 25 "fake" peers via a script sending discovery responses. Verify that the oldest peers are removed and communications continue to work for the 20 most recent ones.
3. **Packet Delivery**: Disconnect a sensor and send a command; verify the receiver reports a "FAIL" status. Reconnect and verify "OK".
4. **Performance**: Monitor the time between receiving a Serial command and the activity LED flashing to ensure low latency under load.


# Delivery Steps

###   Step 1: Memory & Stability: Static Buffers and Watchdog
Replace the dynamic `String` buffer with a fixed-size char array and enable the Hardware Watchdog Timer.

- Change `serialBuffer` to a static `char` array (e.g., 256 bytes) in `main.cpp`.
- Update `processSerialInput` to use indexing and boundary checks instead of `String` concatenation.
- Initialize the ESP32 Task Watchdog Timer (TWDT) in `setup()` to ensure automatic recovery from hangs.

###   Step 2: Scalability: LRU Peer Management
Implement a Least-Recently Used (LRU) mechanism to manage ESP-NOW peers, allowing the system to scale beyond the 20-peer hardware limit.

- Create a `PeerManager` class or set of functions to track active peers.
- Implement logic in `addPeer` to check if the 20-peer limit is reached.
- If the limit is reached, use `esp_now_del_peer` to remove the least recently used peer before adding a new one.
- Update `sendPing` and `sendToDevice` to update the "last used" timestamp for the target peer.

###   Step 3: Monitoring: Packet ACKs and RSSI Reporting
Enhance communication feedback by monitoring packet delivery and reporting signal strength.

- Register `esp_now_register_send_cb` to receive transmission status (ACK/NACK).
- Print a Serial message (e.g., `MSG,status,device,OK/FAIL`) when a packet delivery status is received.
- Update `onReceive` to capture RSSI from the received packet metadata and include it in the Serial output for `DATA` and `PONG` messages.

###   Step 4: Performance: FreeRTOS Task Separation
Refactor the main loop into dedicated FreeRTOS tasks to improve responsiveness and prevent Serial I/O from blocking wireless processing.

- Create a `SerialTask` on Core 0 to handle Serial input reading and command parsing.
- Create a `PacketProcessorTask` on Core 1 to handle ESP-NOW queue processing and Serial output.
- Use FreeRTOS queues or semaphores for thread-safe communication between tasks.
- Move the heartbeat/activity LED logic to a background task or timer.