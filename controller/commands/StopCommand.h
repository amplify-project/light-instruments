#ifndef STOP_COMMAND_H
#define STOP_COMMAND_H

#include "LightCommand.h"

class StopCommand : public LightCommand {
public:
  void execute(const CommandPacket& packet) override;
};

#endif // STOP_COMMAND_H
