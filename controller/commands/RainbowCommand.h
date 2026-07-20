#ifndef RAINBOW_COMMAND_H
#define RAINBOW_COMMAND_H

#include "LightCommand.h"

class RainbowCommand : public LightCommand {
public:
    void execute(const JsonDocument& doc) override;
    void update() override;

private:
    struct RainbowAnim {
        bool active = false;
        char targetPort[16] = "";
        uint8_t initialHue = 0;
        uint8_t deltaHue = 5;
        uint32_t lastUpdate = 0;
    } rainbowAnim;
};

#endif // RAINBOW_COMMAND_H
