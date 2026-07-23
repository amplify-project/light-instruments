#ifndef SET_COMMAND_H
#define SET_COMMAND_H

#include "LightCommand.h"

class SetCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
};

#endif // SET_COMMAND_H
