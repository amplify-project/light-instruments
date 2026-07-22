#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <mutex>
#include "commands/LightCommand.h"

class CommandManager {
public:
  void registerCommand(const std::string& name, std::unique_ptr<LightCommand> cmd);
  void process(const JsonDocument& doc);
  void update();

private:
  std::map<std::string, std::unique_ptr<LightCommand>> commands;
  std::mutex mtx;
};

#endif // COMMAND_MANAGER_H
