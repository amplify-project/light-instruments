#ifndef LIGHT_COMMAND_H
#define LIGHT_COMMAND_H

#include <FastLED.h>
#include <string>

#include "../Globals.h"
#include "../Protocol.h"

class LightCommand {
public:
  virtual ~LightCommand() = default;
  virtual void execute(const CommandPacket& packet) = 0;
  virtual void update() {}

protected:
  bool isTargetPort(const CommandPacket& packet, const std::string& target) {
    const char* port = packet.port;
    return port[0] == '\0' || target == port;
  }
};

#endif // LIGHT_COMMAND_H
