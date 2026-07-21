#ifndef LIGHT_COMMAND_H
#define LIGHT_COMMAND_H

#include "../Globals.h"
#include <ArduinoJson.h>
#include <FastLED.h>
#include <string>

class LightCommand {
public:
    virtual ~LightCommand() = default;
    virtual void execute(const JsonDocument& doc) = 0;
    virtual void update() {}

protected:
    bool isTargetPort(const JsonDocument& doc, const std::string& target) {
        const char* port = doc["port"];
        return port == nullptr || port[0] == '\0' || target == port;
    }
};

#endif // LIGHT_COMMAND_H
