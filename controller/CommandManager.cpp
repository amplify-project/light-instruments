#include "CommandManager.h"

void CommandManager::registerCommand(const std::string& name, std::unique_ptr<LightCommand> cmd) {
  commands[name] = std::move(cmd);
}

void CommandManager::process(const CommandPacket& packet) {
  std::lock_guard<std::mutex> lock(mtx);
  const char* cmdName = packet.command;

  if (cmdName && commands.count(cmdName)) {
    commands[cmdName]->execute(packet);
  }
}

void CommandManager::update() {
  bool needsShow = false;

  {
    std::lock_guard<std::mutex> lock(mtx);

    for (int i = 0; i < numLedStrips; i++) {
      if (ledStrips[i].activeAnimation) {
        if (ledStrips[i].activeAnimation->update(i)) {
          needsShow = true;
        }

        if (ledStrips[i].activeAnimation && ledStrips[i].activeAnimation->isFinished()) {
          ledStrips[i].activeAnimation.reset();
          needsShow = true;
        }
      }
    }
  }

  if (needsShow) {
    triggerDisplay();
  }
}
