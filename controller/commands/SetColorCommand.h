#ifndef SET_COLOR_COMMAND_H
#define SET_COLOR_COMMAND_H

#include "LightCommand.h"

class SetColorCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
};

#endif // SET_COLOR_COMMAND_H
