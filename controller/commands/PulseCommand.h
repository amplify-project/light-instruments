#ifndef PULSE_COMMAND_H
#define PULSE_COMMAND_H

#include "LightCommand.h"
#include "../animations/PulseAnimation.h"

class PulseCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
  void update() override {}
};

#endif // PULSE_COMMAND_H
