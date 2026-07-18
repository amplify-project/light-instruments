#include "CommandManager.h"

void CommandManager::registerCommand(const std::string& name, std::unique_ptr<LightCommand> cmd) {
    commands[name] = std::move(cmd);
}

void CommandManager::process(const JsonDocument& doc) {
    const char* cmdName = doc["command"];

    if (cmdName && commands.count(cmdName)) {
        activeCommand = commands[cmdName].get();
        activeCommand->execute(doc);
    }
}

void CommandManager::update() {
    if (activeCommand) {
        activeCommand->update();
    }
}
