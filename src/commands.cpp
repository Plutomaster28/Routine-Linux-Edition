#include "commands.hpp"
#include "module_loader.hpp"
#include "lua_module.hpp"
#include "script_engine.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

namespace discord {

// Store bot start time
static auto bot_start_time = std::chrono::steady_clock::now();

void Commands::register_all(CommandHandler& handler, DiscordBot& bot) {
    // === Core Commands ===
    handler.register_command("ping", "Check whether Routine is alive", false,
                             [&bot](const json& message, const std::string& args) {
        ping_command(bot, message, args);
    });
    
    handler.register_command("echo", "Repeat a message through Routine", true,
                             [&bot](const json& message, const std::string& args) {
        echo_command(bot, message, args);
    });
    
    handler.register_command("help", "Show the complete Routine command list", false,
                             [&bot](const json& message, const std::string& args) {
        help_command(bot, message, args);
    });
    
    handler.register_command("bench", "Run the C++ kernel benchmark", false,
                             [&bot](const json& message, const std::string& args) {
        benchmark_command(bot, message, args);
    });
    
    // === Kernel Commands ===
    handler.register_command("version", "Show kernel and build information", false,
                             [&bot](const json& message, const std::string& args) {
        version_command(bot, message, args);
    });
    
    handler.register_command("uptime", "Show how long Routine has been running", false,
                             [&bot](const json& message, const std::string& args) {
        uptime_command(bot, message, args);
    });
    
    handler.register_command("status", "Show current kernel runtime statistics", false,
                             [&bot](const json& message, const std::string& args) {
        status_command(bot, message, args);
    });
    
    handler.register_command("reload", "Reload a dynamic Routine module", true,
                             [&bot](const json& message, const std::string& args) {
        reload_command(bot, message, args);
    });
    
    handler.register_command("list", "List loaded modules and extensions", false,
                             [&bot](const json& message, const std::string& args) {
        list_command(bot, message, args);
    });
    
    std::cout << "\n✓ All kernel commands registered\n" << std::endl;
}

void Commands::ping_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    bot.send_message(channel_id, " Pong!");
}

void Commands::echo_command(DiscordBot& bot, const json& message, const std::string& args) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    if (args.empty()) {
        bot.send_message(channel_id, " You need to provide something to echo!");
        return;
    }
    
    bot.send_message(channel_id, " " + args);
}

void Commands::help_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    std::stringstream help_text;
    help_text << "**Routine Bot - Command List**\n\n";
    
    // Core Commands
    help_text << "**Core Commands:**\n";
    help_text << "• `/ping` - Check if the bot is alive\n";
    help_text << "• `/echo` - Bot repeats your message\n";
    help_text << "• `/bench` - Show C++ performance metrics\n";
    help_text << "• `/help` - Show this help message\n\n";
    
    // Kernel Commands
    help_text << "**Kernel Commands:**\n";
    help_text << "• `/version` - Show kernel version and build info\n";
    help_text << "• `/uptime` - Show how long bot has been running\n";
    help_text << "• `/status` - Show runtime statistics\n";
    help_text << "• `/reload` - Hot-reload modules\n";
    help_text << "• `/list` - List loaded modules\n\n";
    
    // Native Module Commands
    auto native_modules = bot.get_module_loader()->get_loaded_modules();
    if (!native_modules.empty()) {
        help_text << "**Native Module Commands:**\n";
        for (const auto& mod_name : native_modules) {
            ModuleInfo* info = bot.get_module_loader()->get_module_info(mod_name);
            if (info) {
                help_text << "*" << info->name << " v" << info->version << "* - " << info->description << "\n";
            } else {
                help_text << "*" << mod_name << "*\n";
            }
            
            // Get commands from this module
            auto cmds = bot.get_module_loader()->get_module_commands(mod_name);
            for (const auto& cmd : cmds) {
                help_text << "• `/" << cmd << "`\n";
            }
        }
        help_text << "\n";
    }
    
    // Lua Module Commands
    auto lua_modules = bot.get_lua_module_loader()->get_loaded_modules();
    if (!lua_modules.empty()) {
        help_text << "**Lua Module Commands:**\n";
        for (const auto& mod_name : lua_modules) {
            help_text << "*" << mod_name << "*\n";
            
            // Get commands from this module
            auto cmds = bot.get_lua_module_loader()->get_module_commands(mod_name);
            for (const auto& cmd : cmds) {
                help_text << "• `/" << cmd << "`\n";
            }
        }
        help_text << "\n";
    }
    
    if (native_modules.empty() && lua_modules.empty()) {
        help_text << "*No custom modules loaded*\n";
        help_text << "Place modules in `modules/` folder\n\n";
    }
    
    help_text << "*Legacy `~` prefix commands remain available.*\n\n";
    help_text << "*Made with organic, fat-free C++*";
    
    bot.send_message(channel_id, help_text.str());
}

void Commands::benchmark_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform some operations to measure
    int iterations = 1000000;
    volatile long long sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::string bench_text = 
        "**C++ Performance Benchmark**\n\n"
        "• Computed sum of 1,000,000 integers\n"
        "• Time taken: **" + std::to_string(duration.count()) + "μs** (" + 
        std::to_string(duration.count() / 1000.0) + "ms)\n"
        "• Operations per second: **" + std::to_string((iterations * 1000000.0) / duration.count()) + "**\n\n"
        "*This is why C++ is king*";
    
    bot.send_message(channel_id, bench_text);
}

// === Kernel Commands Implementation ===

void Commands::version_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    std::string version_text = 
        "**Routine Bot - Kernel Information**\n\n"
        "**Kernel Version:** 1.0.0\n"
        "**Build Date:** " __DATE__ " " __TIME__ "\n"
        "**Compiler:** "
#ifdef __clang__
        "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__)
#elif defined(__GNUC__)
        "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__)
#elif defined(_MSC_VER)
        "MSVC " + std::to_string(_MSC_VER)
#else
        "Unknown"
#endif
        + "\n"
        "**C++ Standard:** C++17\n"
        "**Architecture:** "
#ifdef _WIN64
        "x64 (Windows)"
#elif defined(_WIN32)
        "x86 (Windows)"
#elif defined(__linux__)
        "Linux"
#elif defined(__APPLE__)
        "macOS"
#else
        "Unknown"
#endif
        + "\n"
        "**Libraries:**\n"
        "• libwebsockets (Gateway)\n"
        "• libcurl (REST API)\n"
        "• nlohmann/json (JSON parsing)\n\n"
        "*Pure C++ kernel with no bloat*\n\n"
        "||Miyamii was here||";
    
    bot.send_message(channel_id, version_text);
}

void Commands::uptime_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto uptime_duration = std::chrono::duration_cast<std::chrono::seconds>(now - bot_start_time);
    
    int days = uptime_duration.count() / 86400;
    int hours = (uptime_duration.count() % 86400) / 3600;
    int minutes = (uptime_duration.count() % 3600) / 60;
    int seconds = uptime_duration.count() % 60;
    
    std::stringstream uptime_text;
    uptime_text << "**Bot Uptime** \n\n";
    
    if (days > 0) {
        uptime_text << "**" << days << "** day" << (days != 1 ? "s" : "") << ", ";
    }
    if (hours > 0 || days > 0) {
        uptime_text << "**" << hours << "** hour" << (hours != 1 ? "s" : "") << ", ";
    }
    uptime_text << "**" << minutes << "** minute" << (minutes != 1 ? "s" : "") << ", ";
    uptime_text << "**" << seconds << "** second" << (seconds != 1 ? "s" : "");
    
    uptime_text << "\n\n*Total: " << uptime_duration.count() << " seconds*";
    
    bot.send_message(channel_id, uptime_text.str());
}

void Commands::status_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    // Get memory usage
    size_t memory_kb = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        memory_kb = pmc.WorkingSetSize / 1024;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        memory_kb = usage.ru_maxrss;
    }
#endif
    
    double memory_mb = memory_kb / 1024.0;
    
    std::stringstream status_text;
    status_text << "**Runtime Status** \n\n";
    status_text << "**Memory Usage:** " << std::fixed << std::setprecision(2) << memory_mb << " MB\n";
    const size_t native_count = bot.get_module_loader()->get_loaded_modules().size();
    const size_t lua_count = bot.get_lua_module_loader()->get_loaded_modules().size();
    status_text << "**Native Modules:** " << native_count << "\n";
    status_text << "**Lua Modules:** " << lua_count << "\n";
    status_text << "**Gateway Status:** "
                << (bot.is_connected() ? "Connected" : "Disconnected") << "\n";
    status_text << "**HTTP Client:** Active\n\n";
    status_text << "*All systems operational*";
    
    bot.send_message(channel_id, status_text.str());
}

void Commands::reload_command(DiscordBot& bot, const json& message, const std::string& args) {
    std::string channel_id = message.value("channel_id", "");
    const std::string user_id =
        message.value("author", json::object()).value("id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    if (user_id.empty() || !bot.is_guild_admin(channel_id, user_id)) {
        bot.send_message(channel_id,
            "Module reload requires guild ownership, Administrator, or Manage Server.");
        return;
    }
    
    std::stringstream reload_text;
    reload_text << "**Module Reload**\n\n";
    
    if (args.empty()) {
        // Reload all modules AND scan for new ones
        reload_text << "**Reloading all modules and scanning for new ones...**\n\n";
        
        // First, scan for new modules in the same directories
        std::vector<std::string> module_paths = {
            "modules",
            "../modules",
            "../../modules"
        };
        
        size_t new_native = 0, new_lua = 0;
        for (const auto& path : module_paths) {
            size_t before_native = bot.get_module_loader()->get_loaded_modules().size();
            size_t before_lua = bot.get_lua_module_loader()->get_loaded_modules().size();
            
            bot.get_module_loader()->load_modules_from_directory(path);
            bot.get_lua_module_loader()->load_modules_from_directory(path);
            
            size_t after_native = bot.get_module_loader()->get_loaded_modules().size();
            size_t after_lua = bot.get_lua_module_loader()->get_loaded_modules().size();
            
            new_native += (after_native - before_native);
            new_lua += (after_lua - before_lua);
            
            if (new_native > 0 || new_lua > 0) {
                break; // Found the right directory
            }
        }
        
        if (new_native > 0 || new_lua > 0) {
            reload_text << "**New modules found:**\n";
            if (new_native > 0) reload_text << "• " << new_native << " native module(s)\n";
            if (new_lua > 0) reload_text << "• " << new_lua << " Lua module(s)\n";
            reload_text << "\n";
        }
        
        auto native_modules = bot.get_module_loader()->get_loaded_modules();
        auto lua_modules = bot.get_lua_module_loader()->get_loaded_modules();
        
        int success = 0, failed = 0;
        
        // Reload native modules
        for (const auto& mod : native_modules) {
            if (bot.get_module_loader()->reload_module(mod)) {
                reload_text << "Successfully reloaded " << mod << " (native)\n";
                success++;
            } else {
                reload_text << "Failed to reload " << mod << " (native) - failed\n";
                failed++;
            }
        }
        
        // Reload Lua modules
        for (const auto& mod : lua_modules) {
            if (bot.get_lua_module_loader()->reload_module(mod)) {
                reload_text << "Successfully reloaded " << mod << " (Lua)\n";
                success++;
            } else {
                reload_text << "Failed to reload " << mod << " (Lua) - failed\n";
                failed++;
            }
        }
        
        reload_text << "\n**Summary:** " << success << " reloaded, " << failed << " failed";
        if (new_native > 0 || new_lua > 0) {
            reload_text << ", " << (new_native + new_lua) << " new modules loaded";
        }
    } else {
        // Reload specific module
        std::string module_name = args;
        
        // Try native modules first
        if (bot.get_module_loader()->is_module_loaded(module_name)) {
            if (bot.get_module_loader()->reload_module(module_name)) {
                reload_text << "Successfully reloaded `" << module_name << "` (native)";
            } else {
                reload_text << "Failed to reload `" << module_name << "` (native)";
            }
        }
        // Try Lua modules
        else if (bot.get_lua_module_loader()->is_module_loaded(module_name)) {
            if (bot.get_lua_module_loader()->reload_module(module_name)) {
                reload_text << "Successfully reloaded `" << module_name << "` (Lua)";
            } else {
                reload_text << "Failed to reload `" << module_name << "` (Lua)";
            }
        } else {
            reload_text << "Module `" << module_name << "` not found\n\n";
            reload_text << "Use `~list` to see loaded modules";
        }
    }
    
    bot.send_message(channel_id, reload_text.str());
}

void Commands::list_command(DiscordBot& bot, const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    
    if (channel_id.empty()) {
        std::cerr << "No channel_id in message" << std::endl;
        return;
    }
    
    std::stringstream list_text;
    list_text << "**Loaded Modules**\n\n";
    
    // Kernel modules (built-in)
    list_text << "**Kernel Modules (built-in):**\n";
    list_text << "• `core` - Basic commands (ping, echo, help)\n";
    list_text << "• `kernel` - System commands (version, uptime, status)\n";
    list_text << "• `bench` - Performance testing\n\n";
    
    // Native compiled modules
    auto native_modules = bot.get_module_loader()->get_loaded_modules();
    if (!native_modules.empty()) {
        list_text << "**Native Modules (" << native_modules.size() << "):**\n";
        for (const auto& mod_name : native_modules) {
            ModuleInfo* info = bot.get_module_loader()->get_module_info(mod_name);
            if (info) {
                list_text << "• `" << mod_name << "` v" << info->version 
                         << " - " << info->description << "\n";
            } else {
                list_text << "• `" << mod_name << "`\n";
            }
        }
        list_text << "\n";
    }
    
    // Lua modules
    auto lua_modules = bot.get_lua_module_loader()->get_loaded_modules();
    if (!lua_modules.empty()) {
        list_text << "**Lua Modules (" << lua_modules.size() << "):**\n";
        for (const auto& mod_name : lua_modules) {
            list_text << "• `" << mod_name << "` (Lua)\n";
        }
        list_text << "\n";
    }
    
    if (native_modules.empty() && lua_modules.empty()) {
        list_text << "**Custom Modules:**\n";
        list_text << "• No modules loaded\n\n";
        list_text << "*Place modules in the `modules/` folder*\n";
    }
    
    list_text << "*Use `~reload <module>` to reload a module*";
    
    bot.send_message(channel_id, list_text.str());
}

// === Script Commands ===

void Commands::register_script_commands(CommandHandler& handler, DiscordBot& bot, ScriptEngine& engine) {
    handler.register_command("script", [&bot, &engine](const json& message, const std::string& args) {
        // Parse subcommand - handle both space and newline separators
        std::string subcommand;
        std::string rest;
        
        // Find first space or newline
        size_t sep_pos = args.find_first_of(" \n");
        if (sep_pos != std::string::npos) {
            subcommand = args.substr(0, sep_pos);
            rest = args.substr(sep_pos + 1);
        } else {
            subcommand = args;
        }
        
        if (subcommand == "load") {
            script_load_command(bot, engine, message, rest);
        } else if (subcommand == "list") {
            script_list_command(bot, engine, message, rest);
        } else if (subcommand == "enable") {
            script_enable_command(bot, engine, message, rest);
        } else if (subcommand == "disable") {
            script_disable_command(bot, engine, message, rest);
        } else if (subcommand == "remove") {
            script_remove_command(bot, engine, message, rest);
        } else if (subcommand == "show") {
            script_show_command(bot, engine, message, rest);
        } else {
            std::string channel_id = message.value("channel_id", "");
            bot.send_message(channel_id, 
                "**Script Commands:**\n"
                "• `~script load` - Load script from code block\n"
                "• `~script list` - List all scripts\n"
                "• `~script show <name>` - Show script details\n"
                "• `~script enable <name>` - Enable a script\n"
                "• `~script disable <name>` - Disable a script\n"
                "• `~script remove <name>` - Remove a script");
        }
    });
    
    std::cout << "✓ Script commands registered\n" << std::endl;
}

void Commands::script_load_command(DiscordBot& bot, ScriptEngine& engine, 
                                   const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    std::string content = message.value("content", "");
    std::string author_id = message.value("author", json::object()).value("id", "");
    std::string author_name = message.value("author", json::object()).value("username", "");
    
    if (channel_id.empty()) return;
    
    // First, try to find a code block
    size_t code_block_start = content.find("```");
    std::string script_text;
    
    if (code_block_start != std::string::npos) {
        // Code block found - extract it
        size_t start = code_block_start + 3;
        
        // Skip language identifier if present (```yaml, ```txt, etc.)
        if (start < content.length() && content[start] != '\n') {
            start = content.find('\n', start);
            if (start == std::string::npos) {
                bot.send_message(channel_id, "Invalid code block format");
                return;
            }
            start += 1;
        }
        
        size_t end = content.find("```", start);
        if (end == std::string::npos) {
            bot.send_message(channel_id, "Code block not properly closed");
            return;
        }
        
        script_text = content.substr(start, end - start);
    } else {
        // No code block - try to extract YAML directly after "~script load"
        size_t load_pos = content.find("~script load");
        if (load_pos == std::string::npos) {
            bot.send_message(channel_id, "Could not parse command");
            return;
        }
        
        // Find the newline after "~script load"
        size_t yaml_start = content.find('\n', load_pos);
        if (yaml_start == std::string::npos || yaml_start + 1 >= content.length()) {
            bot.send_message(channel_id, 
                "No script content found.\n\n**Usage:**\n```\n~script load\nscript: your_script\non: message.create\ndo:\n  - module: responder\n    args:\n      channel: \"{{channel_id}}\"\n      content: \"Hello!\"\n```");
            return;
        }
        
        // Extract everything after "~script load\n"
        script_text = content.substr(yaml_start + 1);
        
        // Trim trailing whitespace
        size_t last_char = script_text.find_last_not_of(" \t\r\n");
        if (last_char != std::string::npos) {
            script_text = script_text.substr(0, last_char + 1);
        }
    }
    
    // Load the script
    std::string error;
    if (engine.load_script(script_text, author_name, error)) {
        bot.send_message(channel_id, "Script loaded successfully!");
    } else {
        bot.send_message(channel_id, "Failed to load script:\n```\n" + error + "\n```");
    }
}

void Commands::script_list_command(DiscordBot& bot, ScriptEngine& engine,
                                   const json& message, const std::string& /*args*/) {
    std::string channel_id = message.value("channel_id", "");
    if (channel_id.empty()) return;
    
    auto scripts = engine.list_scripts();
    
    if (scripts.empty()) {
        bot.send_message(channel_id, "**Scripts:** None loaded");
        return;
    }
    
    std::stringstream ss;
    ss << "**Loaded Scripts (" << scripts.size() << "):**\n\n";
    
    for (const auto& name : scripts) {
        const Script* script = engine.get_script(name);
        if (script) {
            ss << "• `" << name << "` - " << script->event_type;
            if (!script->enabled) {
                ss << " (disabled)";
            }
            ss << "\n";
            ss << "  *by " << script->author << "*\n";
        }
    }
    
    ss << "\n*Use `~script show <name>` for details*";
    
    bot.send_message(channel_id, ss.str());
}

void Commands::script_enable_command(DiscordBot& bot, ScriptEngine& engine,
                                     const json& message, const std::string& args) {
    std::string channel_id = message.value("channel_id", "");
    if (channel_id.empty()) return;
    
    if (args.empty()) {
        bot.send_message(channel_id, "Usage: `~script enable <name>`");
        return;
    }
    
    if (engine.enable_script(args)) {
        bot.send_message(channel_id, "Script `" + args + "` enabled");
    } else {
        bot.send_message(channel_id, "Script `" + args + "` not found");
    }
}

void Commands::script_disable_command(DiscordBot& bot, ScriptEngine& engine,
                                      const json& message, const std::string& args) {
    std::string channel_id = message.value("channel_id", "");
    if (channel_id.empty()) return;
    
    if (args.empty()) {
        bot.send_message(channel_id, "Usage: `~script disable <name>`");
        return;
    }
    
    if (engine.disable_script(args)) {
        bot.send_message(channel_id, "Script `" + args + "` disabled");
    } else {
        bot.send_message(channel_id, "Script `" + args + "` not found");
    }
}

void Commands::script_remove_command(DiscordBot& bot, ScriptEngine& engine,
                                     const json& message, const std::string& args) {
    std::string channel_id = message.value("channel_id", "");
    if (channel_id.empty()) return;
    
    if (args.empty()) {
        bot.send_message(channel_id, "Usage: `~script remove <name>`");
        return;
    }
    
    if (engine.remove_script(args)) {
        bot.send_message(channel_id, "Script `" + args + "` removed");
    } else {
        bot.send_message(channel_id, "Script `" + args + "` not found");
    }
}

void Commands::script_show_command(DiscordBot& bot, ScriptEngine& engine,
                                   const json& message, const std::string& args) {
    std::string channel_id = message.value("channel_id", "");
    if (channel_id.empty()) return;
    
    if (args.empty()) {
        bot.send_message(channel_id, "Usage: `~script show <name>`");
        return;
    }
    
    const Script* script = engine.get_script(args);
    if (!script) {
        bot.send_message(channel_id, "Script `" + args + "` not found");
        return;
    }
    
    std::stringstream ss;
    ss << "**Script: " << script->name << "**\n";
    ss << "Event: `" << script->event_type << "`\n";
    ss << "Status: " << (script->enabled ? "Enabled" : "Disabled") << "\n";
    ss << "Author: " << script->author << "\n";
    ss << "Created: " << script->created_at << "\n\n";
    
    if (!script->conditions.empty()) {
        ss << "**Conditions:**\n";
        for (const auto& cond : script->conditions) {
            ss << "• " << cond.value << "\n";
        }
        ss << "\n";
    }
    
    ss << "**Actions (" << script->actions.size() << "):**\n";
    for (const auto& action : script->actions) {
        ss << "• Module: `" << action.module << "`\n";
    }
    
    bot.send_message(channel_id, ss.str());
}

} // namespace discord
