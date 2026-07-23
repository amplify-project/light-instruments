#ifndef FIRE_COMMAND_H
#define FIRE_COMMAND_H

#include "LightCommand.h"
#include "../animations/FireAnimation.h"

class FireCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
  void update() override {}
};

#endif // FIRE_COMMAND_H
