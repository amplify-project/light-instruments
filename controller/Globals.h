#ifndef GLOBALS_H
#define GLOBALS_H

#include "HardwareConfig.h"
#include "LedStrip.h"
#include <vector>

class CommandManager;

extern std::vector<LedStrip> ledStrips;
extern CommandManager commandManager;

#define numLedStrips ((int)ledStrips.size())

void showStrip(int index);
void showAll();
void addLedStrip(uint8_t port, int numLeds, const char* name = nullptr);

#endif // GLOBALS_H
