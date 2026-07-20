#ifndef COMET_COMMAND_H
#define COMET_COMMAND_H

#include "LightCommand.h"

class CometCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct CometAnim {
        bool active = false;
        char targetPort[16] = "";
        CRGB color = CRGB::White;
        uint32_t speed = 0;
        uint32_t lastUpdate = 0;
        float position = 0;
    } cometAnim;
};

#endif // COMET_COMMAND_H
