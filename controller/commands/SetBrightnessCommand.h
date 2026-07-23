#ifndef SET_BRIGHTNESS_COMMAND_H
#define SET_BRIGHTNESS_COMMAND_H

#include "LightCommand.h"

class SetBrightnessCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
};

#endif // SET_BRIGHTNESS_COMMAND_H
