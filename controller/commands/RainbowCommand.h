#ifndef RAINBOW_COMMAND_H
#define RAINBOW_COMMAND_H

#include "LightCommand.h"
#include "../animations/RainbowAnimation.h"

class RainbowCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
  void update() override {}
};

#endif // RAINBOW_COMMAND_H
