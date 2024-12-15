#include "commands.h"

#include <iostream>
#include <userid.hpp>
#include "session.h"

extern qls::Session session;

namespace qls
{

class HelpCommand: public Command
{
public:
    HelpCommand(CommandManager& cm):
        m_command_manager(cm) {}
    ~HelpCommand() = default;

    opt::Option getOption() const
    {
        opt::Option option;
        option.add("name", opt::Option::OptionType::OPT_OPTIONAL);
        return option;
    }

    void execute(const opt::Option& option)
    {
        if (option.has_opt_with_value("name"))
        {
            std::string commandName = option.get_string("name");
            if (!m_command_manager.canFindCommand(commandName))
            {
                std::cout << "Could not find command: " << commandName << '\n';
            }
            m_command_manager.getCommand(commandName)->getOption().show();
            return;
        }

        std::shared_lock<std::shared_mutex> local_shared_lock(m_command_manager.m_command_map_mutex);
        std::cout << "help [--name=(function name)]\n";
        for (const auto& [commandName, command_ptr]: std::as_const(m_command_manager.m_command_map))
        {
            std::cout << commandName << '\n';
        }
    }

private:
    CommandManager& m_command_manager;
};

class ExitCommand: public Command
{
public:
    ExitCommand() = default;
    ~ExitCommand() = default;

    opt::Option getOption() const
    {
        return {};
    }

    void execute(const opt::Option& option)
    {
        exit(0);
    }
};

class RegisterUserCommand: public Command
{
public:
    RegisterUserCommand() = default;
    ~RegisterUserCommand() = default;

    opt::Option getOption() const
    {
        opt::Option opt;
        opt.add("email", opt::Option::OptionType::OPT_REQUIRED);
        opt.add("password", opt::Option::OptionType::OPT_REQUIRED);
        return opt;
    }

    void execute(const opt::Option& opt)
    {
        qls::UserID user_id;
        session.registerUser(opt.get_string("email"), opt.get_string("password"), user_id);
    }
};

class LoginUserCommand: public Command
{
public:
    LoginUserCommand() = default;
    ~LoginUserCommand() = default;

    opt::Option getOption() const
    {
        opt::Option opt;
        opt.add("userid", opt::Option::OptionType::OPT_REQUIRED);
        opt.add("password", opt::Option::OptionType::OPT_REQUIRED);
        return opt;
    }

    void execute(const opt::Option& opt)
    {
        session.loginUser(qls::UserID(opt.get_int("userid")), opt.get_string("password"));
    }
};

class CreateFriendApplication: public Command
{
public:
    CreateFriendApplication() = default;
    ~CreateFriendApplication() = default;

    opt::Option getOption() const
    {
        opt::Option opt;
        opt.add("userid", opt::Option::OptionType::OPT_REQUIRED);
        return opt;
    }

    void execute(const opt::Option& opt)
    {
        session.createFriendApplication(qls::UserID(opt.get_int("userid")));
    }
};

class ApplyFriendApplication: public Command
{
public:
    ApplyFriendApplication() = default;
    ~ApplyFriendApplication() = default;

    opt::Option getOption() const
    {
        opt::Option opt;
        opt.add("userid", opt::Option::OptionType::OPT_REQUIRED);
        return opt;
    }

    void execute(const opt::Option& opt)
    {
        session.applyFriendApplication(qls::UserID(opt.get_int("userid")));
    }
};

class RejectFriendApplication: public Command
{
public:
    RejectFriendApplication() = default;
    ~RejectFriendApplication() = default;

    opt::Option getOption() const
    {
        opt::Option opt;
        opt.add("userid", opt::Option::OptionType::OPT_REQUIRED);
        return opt;
    }

    void execute(const opt::Option& opt)
    {
        session.rejectFriendApplication(qls::UserID(opt.get_int("userid")));
    }
};

class CreateGroupApplication: public Command
{
public:
    CreateGroupApplication() = default;
    ~CreateGroupApplication() = default;

    opt::Option getOption() const
    {
        opt::Option opt;
        opt.add("groupid", opt::Option::OptionType::OPT_REQUIRED);
        return opt;
    }

    void execute(const opt::Option& opt)
    {
        session.createGroupApplication(qls::GroupID(opt.get_int("groupid")));
    }
};

CommandManager::CommandManager()
{
    addCommand("exit", std::make_shared<ExitCommand>());
    addCommand("help", std::make_shared<HelpCommand>(*this));
    addCommand("registerUser", std::make_shared<RegisterUserCommand>());
    addCommand("loginUser", std::make_shared<LoginUserCommand>());
    addCommand("createFriendApplication", std::make_shared<CreateFriendApplication>());
    addCommand("applyFriendApplication", std::make_shared<ApplyFriendApplication>());
    addCommand("rejectFriendApplication", std::make_shared<RejectFriendApplication>());
    addCommand("createGroupApplication", std::make_shared<CreateGroupApplication>());
}

bool CommandManager::addCommand(std::string_view commandName, const std::shared_ptr<Command> &command_ptr)
{
    std::unique_lock<std::shared_mutex> local_unique_lock(m_command_map_mutex);
    if (m_command_map.find(commandName) != m_command_map.cend()) {
        return false;
    }
    m_command_map.emplace(commandName, command_ptr);
    return true;
}

bool CommandManager::removeCommand(std::string_view commandName)
{
    std::unique_lock<std::shared_mutex> local_unique_lock(m_command_map_mutex);
    auto iter = m_command_map.find(commandName);
    if (iter == m_command_map.cend()) {
        return false;
    }
    m_command_map.erase(iter);
    return true;
}

bool CommandManager::canFindCommand(std::string_view commandName) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_command_map_mutex);
    return m_command_map.find(commandName) != m_command_map.cend();
}

std::shared_ptr<Command> CommandManager::getCommand(std::string_view commandName) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_command_map_mutex);
    auto iter = m_command_map.find(commandName);
    if (iter == m_command_map.cend())
        throw std::logic_error(std::string(commandName) + " does not exist");
    return iter->second;
}

} // namespace qls
