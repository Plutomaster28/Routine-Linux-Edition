#include "command_handler.hpp"
#include "discord_bot.hpp"
#include "discord_command_utils.hpp"
#include "discord_interaction_utils.hpp"
#include "module_loader.hpp"
#include "lua_module.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>

namespace discord {
namespace {

std::string slash_input_prompt(const std::string& command) {
    if (command == "echo") return "Text for Routine to repeat";
    if (command == "reload") return "Module name to reload";
    if (command == "script") return "Script action and parameters";
    if (command == "pay") return "Recipient and amount, for example: @user 250";
    if (command == "forex") return "markets, quote SERVER [AMOUNT], or exchange SERVER AMOUNT";
    if (command == "profile") return "Leave blank for local finances, or enter: global";
    if (command == "news") return "all, local, or global";
    if (command == "government") return "Status, tariff 0-25, or treaty SERVER";
    if (command == "business") return "Business action; use export SERVER QUANTITY for trade";
    if (command == "collectible") return "buy/sell, serials, or move SERIAL";
    if (command == "stock") return "Action, ticker, and quantity; leave blank for help";
    if (command == "orders") return "Action and order details; leave blank for help";
    if (command == "econadmin") return "Admin action and values; leave blank for help";
    return "Options, amounts, or targets; leave blank to show command help";
}

}  // namespace

CommandHandler::CommandHandler(const std::string& prefix)
    : prefix_(prefix), bot_(nullptr) {
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::register_command(const std::string& command, CommandCallback callback) {
    register_command(command, "Run the " + command + " command", true,
                     std::move(callback));
}

void CommandHandler::register_command(const std::string& command,
                                      const std::string& description,
                                      bool accepts_arguments,
                                      CommandCallback callback) {
    commands_[command] = {
        std::move(callback), description, accepts_arguments
    };
    std::cout << "✓ Registered command: " << prefix_ << command << std::endl;
}

bool CommandHandler::handle_message(const json& message) {
    try {
        // Get message content
        std::string content = message.value("content", "");
        
        // Check if message starts with prefix
        if (content.empty() || content.substr(0, prefix_.length()) != prefix_) {
            return false;
        }
        
        // Parse command and arguments
        std::string command, args;
        if (!parse_prefixed_command(content, prefix_, command, args)) {
            return false;
        }
        
        // Find and execute command
        auto it = commands_.find(command);
        if (it != commands_.end()) {
            std::cout << "→ Executing command: " << prefix_ << command << std::endl;
            it->second.callback(message, args);
            return true;
        }
        
        // Check module commands if bot is set
        if (bot_) {
            std::string channel_id = message.value("channel_id", "");
            std::string guild_id = message.value("guild_id", "");
            std::string user_id = "";
            if (message.contains("author") && message["author"].is_object()) {
                user_id = message["author"].value("id", "");
            }

            if (!channel_id.empty() && !guild_id.empty()) {
                bot_->remember_channel_guild(channel_id, guild_id);
            }
            if (!channel_id.empty() && !user_id.empty() && message.contains("member") &&
                message["member"].is_object() && message["member"].contains("roles") &&
                message["member"]["roles"].is_array()) {
                std::string roles;
                for (const auto& role : message["member"]["roles"]) {
                    if (!role.is_string()) continue;
                    if (!roles.empty()) roles += ',';
                    roles += role.get<std::string>();
                }
                bot_->remember_user_roles(channel_id, user_id, roles);
            }
            
            std::cout << "→ Checking modules for command: " << command << std::endl;
            
            // Try native modules
            if (bot_->get_module_loader()->dispatch_command(command, channel_id, user_id, args)) {
                std::cout << "→ Executed native module command: " << prefix_ << command << std::endl;
                return true;
            }
            
            // Try Lua modules
            if (bot_->get_lua_module_loader()->dispatch_command(command, channel_id, user_id, args)) {
                std::cout << "→ Executed Lua module command: " << prefix_ << command << std::endl;
                return true;
            }
            
            std::cout << "→ Command not found in modules: " << command << std::endl;
        } else {
            std::cout << "→ Bot not set, cannot check modules" << std::endl;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling command: " << e.what() << std::endl;
        return false;
    }
}

std::string CommandHandler::interaction_arguments(const json& data) {
    return render_interaction_arguments(data);
}

bool CommandHandler::handle_interaction(const json& interaction) {
    try {
        if (interaction.value("type", 0) != 2 ||
            !interaction.contains("data") ||
            !interaction["data"].is_object() || !bot_) {
            return false;
        }

        const json& data = interaction["data"];
        std::string command = data.value("name", "");
        std::transform(command.begin(), command.end(), command.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
        const std::string channel_id = interaction.value(
            "channel_id",
            interaction.value("channel", json::object()).value("id", ""));
        const std::string guild_id = interaction.value("guild_id", "");
        const json member = interaction.value("member", json::object());
        const json user = member.value(
            "user", interaction.value("user", json::object()));
        const std::string user_id = user.value("id", "");

        if (command.empty() || channel_id.empty() || user_id.empty()) {
            return false;
        }
        if (!guild_id.empty()) {
            bot_->remember_channel_guild(channel_id, guild_id);
        }
        if (member.contains("roles") && member["roles"].is_array()) {
            std::string roles;
            for (const auto& role : member["roles"]) {
                if (!role.is_string()) continue;
                if (!roles.empty()) roles += ',';
                roles += role.get<std::string>();
            }
            bot_->remember_user_roles(channel_id, user_id, roles);
        }

        if (!bot_->begin_interaction_response(
                interaction.value("id", ""),
                interaction.value("token", ""),
                interaction.value("application_id", ""),
                channel_id)) {
            return false;
        }
        struct InteractionScope {
            DiscordBot* bot;
            std::string channel_id;
            ~InteractionScope() {
                if (!bot->interaction_response_queued()) {
                    bot->send_message(
                        channel_id,
                        "The command stopped before producing a response.");
                }
                bot->end_interaction_response();
            }
        } scope{bot_, channel_id};

        json synthetic_message = {
            {"channel_id", channel_id},
            {"guild_id", guild_id},
            {"author", user},
            {"member", member},
            {"interaction", interaction}
        };
        const std::string args = interaction_arguments(data);

        bool handled = false;
        auto command_it = commands_.find(command);
        if (command_it != commands_.end()) {
            command_it->second.callback(synthetic_message, args);
            handled = true;
        } else if (bot_->get_module_loader()->dispatch_command(
                       command, channel_id, user_id, args)) {
            handled = true;
        } else if (bot_->get_lua_module_loader()->dispatch_command(
                       command, channel_id, user_id, args)) {
            handled = true;
        }

        if (!handled) {
            bot_->send_message(
                channel_id,
                "That application command is no longer registered. "
                "Routine will refresh its command list on restart.");
        } else if (!bot_->interaction_response_queued()) {
            bot_->send_message(channel_id, "Command completed.");
        }
        return handled;
    } catch (const std::exception& error) {
        std::cerr << "Error handling interaction: " << error.what()
                  << std::endl;
        return false;
    }
}

json CommandHandler::build_application_commands() const {
    struct Definition {
        std::string description;
        bool accepts_arguments;
    };
    std::map<std::string, Definition> definitions;
    for (const auto& entry : commands_) {
        definitions.emplace(
            entry.first,
            Definition{entry.second.description,
                       entry.second.accepts_arguments});
    }

    if (bot_) {
        for (const auto& definition :
             bot_->get_module_loader()->get_command_definitions()) {
            definitions.emplace(
                definition.first,
                Definition{definition.second, true});
        }
        for (const std::string& module :
             bot_->get_lua_module_loader()->get_loaded_modules()) {
            for (const std::string& command :
                 bot_->get_lua_module_loader()->get_module_commands(module)) {
                definitions.emplace(
                    command,
                    Definition{"Run the " + command + " module command",
                               true});
            }
        }
    }

    json result = json::array();
    constexpr std::size_t maximum_chat_input_commands = 100;
    for (const auto& entry : definitions) {
        if (result.size() >= maximum_chat_input_commands) {
            std::cerr << "Discord supports at most 100 chat-input commands; "
                         "additional module commands were skipped."
                      << std::endl;
            break;
        }
        const std::string& name = entry.first;
        if (!valid_chat_input_command_name(name)) {
            std::cerr << "Skipping invalid application-command name: "
                      << name << std::endl;
            continue;
        }

        std::string description = entry.second.description;
        if (description.empty()) description = "Routine command";
        if (description.size() > 100) description.resize(100);
        json command = {
            {"name", name},
            {"description", description},
            {"type", 1}
        };
        if (entry.second.accepts_arguments) {
            command["options"] = json::array({
                {
                    {"type", 3},
                    {"name", "input"},
                    {"description", slash_input_prompt(name)},
                    {"required", false},
                    {"max_length", 1000}
                }
            });
        }
        result.push_back(std::move(command));
    }
    return result;
}

} // namespace discord
