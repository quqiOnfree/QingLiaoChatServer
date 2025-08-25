#ifndef COMMANDS_H
#define COMMANDS_H

#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>

#include "session.h"
#include "string_param.hpp"
#include <option.hpp>

namespace qls {

class Command {
public:
  Command() = default;
  virtual ~Command() noexcept = default;

  virtual opt::Option getOption() const = 0;
  virtual void execute(qls::Session &, const opt::Option &) = 0;
};

class HelpCommand;
class CommandManager final {
public:
  CommandManager();
  ~CommandManager() noexcept = default;

  bool addCommand(string_param commandName,
                  const std::shared_ptr<Command> &commandPtr);
  bool removeCommand(string_param commandName);
  bool canFindCommand(string_param commandName) const;
  std::shared_ptr<Command> getCommand(string_param commandName) const;

private:
  std::map<std::string, std::shared_ptr<Command>, std::less<>> m_command_map;
  mutable std::shared_mutex m_command_map_mutex;

  friend class HelpCommand;
};

} // namespace qls

#endif
