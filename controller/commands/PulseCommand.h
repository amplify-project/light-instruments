#ifndef PULSE_COMMAND_H
#define PULSE_COMMAND_H

#include "LightCommand.h"
#include "../animations/PulseAnimation.h"

class PulseCommand : public LightCommand {
public:
  void execute(const JsonDocument& doc) override;
  void update() override {}
};

#endif // PULSE_COMMAND_H
