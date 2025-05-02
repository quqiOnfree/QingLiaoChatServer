#ifndef COMMANDS_H
#define COMMANDS_H

#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <shared_mutex>

#include <option.hpp>
#include "session.h"

namespace qls
{

class Command
{
public:
    Command() = default;
    virtual ~Command() noexcept = default;

    virtual opt::Option getOption() const = 0;
    virtual void execute(qls::Session&, const opt::Option&) = 0;
};

class HelpCommand;
class CommandManager final
{
public:
    CommandManager();
    ~CommandManager() noexcept = default;

    bool addCommand(std::string_view commandName, const std::shared_ptr<Command>& commandPtr);
    bool removeCommand(std::string_view commandName);
    bool canFindCommand(std::string_view commandName) const;
    std::shared_ptr<Command> getCommand(std::string_view commandName) const;

private:
    std::map<std::string, std::shared_ptr<Command>, std::less<>>
                                m_command_map;
    mutable std::shared_mutex   m_command_map_mutex;

    friend class HelpCommand;
};

} // namespace qls

#endif
