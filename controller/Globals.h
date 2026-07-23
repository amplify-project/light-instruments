#ifndef GLOBALS_H
#define GLOBALS_H

#include <vector>

#include "HardwareConfig.h"
#include "LedStrip.h"

class CommandManager;

extern std::vector<LedStrip> ledStrips;
extern CommandManager commandManager;
extern TaskHandle_t displayTaskHandle;

#define numLedStrips ((int)ledStrips.size())

void showStrip(int index);
void showAll();
void triggerDisplay();
void addLedStrip(uint8_t port, int numLeds, const char* name = nullptr);

#endif // GLOBALS_H
