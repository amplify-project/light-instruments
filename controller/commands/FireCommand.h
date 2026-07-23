#ifndef FIRE_COMMAND_H
#define FIRE_COMMAND_H

#include "LightCommand.h"
#include "../animations/FireAnimation.h"

class FireCommand : public LightCommand {
public:
  void execute(const JsonDocument& doc) override;
  void update() override {}
};

#endif // FIRE_COMMAND_H
