#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum MessageType : uint8_t {
    MSG_DISCOVERY = 0,
    MSG_DISCOVERY_RESPONSE = 1,
    MSG_PING = 2,
    MSG_PONG = 3,
    MSG_DATA = 4,
    MSG_COMMAND = 5
};

struct __attribute__((packed)) ProtocolHeader {
    uint8_t type;
};

struct __attribute__((packed)) DiscoveryPacket {
    uint8_t type = MSG_DISCOVERY;
};

struct __attribute__((packed)) DiscoveryResponsePacket {
    uint8_t type = MSG_DISCOVERY_RESPONSE;
    char deviceName[16];
    char deviceType[16];
};

struct __attribute__((packed)) PingPacket {
    uint8_t type = MSG_PING;
};

struct __attribute__((packed)) PongPacket {
    uint8_t type = MSG_PONG;
    char deviceName[16];
    char deviceType[16];
};

struct __attribute__((packed)) DataPacket {
    uint8_t type = MSG_DATA;
    char deviceName[16];
    char port[8];
    int32_t value;
};

struct __attribute__((packed)) CommandPacket {
    uint8_t type = MSG_COMMAND;
    char deviceName[16];
    char port[8];
    char command[16];
    char value[32];
};

#endif
