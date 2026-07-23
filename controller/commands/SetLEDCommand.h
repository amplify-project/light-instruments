#ifndef SET_LED_COMMAND_H
#define SET_LED_COMMAND_H

#include "LightCommand.h"

class SetLEDCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
};

#endif // SET_LED_COMMAND_H
