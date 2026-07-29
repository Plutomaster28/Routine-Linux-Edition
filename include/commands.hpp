#pragma once

#include "command_handler.hpp"
#include "discord_bot.hpp"

namespace discord {

class ScriptEngine;  // Forward declaration

class Commands {
public:
    // Register all commands with the command handler
    static void register_all(CommandHandler& handler, DiscordBot& bot);
    
    // Register script commands (requires script engine)
    static void register_script_commands(CommandHandler& handler, DiscordBot& bot, ScriptEngine& engine);
    
private:
    // === Core Commands ===
    
    // Ping command - responds with latency
    static void ping_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // Echo command - repeats what user says
    static void echo_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // Help command - shows available commands
    static void help_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // Benchmark command - shows C++ performance
    static void benchmark_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // === Kernel Commands ===
    
    // Version command - shows kernel version and build info
    static void version_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // Uptime command - shows how long bot has been running
    static void uptime_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // Status command - shows runtime stats
    static void status_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // Reload command - placeholder for hot-reloading config/modules
    static void reload_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // List command - placeholder for listing loaded modules
    static void list_command(DiscordBot& bot, const json& message, const std::string& args);
    
    // === Script Commands ===
    
    // Script load - load a script from a code block
    static void script_load_command(DiscordBot& bot, ScriptEngine& engine, const json& message, const std::string& args);
    
    // Script list - list all loaded scripts
    static void script_list_command(DiscordBot& bot, ScriptEngine& engine, const json& message, const std::string& args);
    
    // Script enable/disable - toggle scripts
    static void script_enable_command(DiscordBot& bot, ScriptEngine& engine, const json& message, const std::string& args);
    static void script_disable_command(DiscordBot& bot, ScriptEngine& engine, const json& message, const std::string& args);
    
    // Script remove - delete a script
    static void script_remove_command(DiscordBot& bot, ScriptEngine& engine, const json& message, const std::string& args);
    
    // Script show - show details of a script
    static void script_show_command(DiscordBot& bot, ScriptEngine& engine, const json& message, const std::string& args);
};

} // namespace discord
