#ifndef BREATHE_COMMAND_H
#define BREATHE_COMMAND_H

#include "LightCommand.h"
#include "../animations/BreatheAnimation.h"

class BreatheCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
  void update() override {}
};

#endif // BREATHE_COMMAND_H
