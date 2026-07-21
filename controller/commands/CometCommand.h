#ifndef COMET_COMMAND_H
#define COMET_COMMAND_H

#include "LightCommand.h"
#include "../animations/CometAnimation.h"

class CometCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override {}
};

#endif // COMET_COMMAND_H
