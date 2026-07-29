#include "discord_bot.hpp"
#include "command_handler.hpp"
#include "commands.hpp"
#include "script_engine.hpp"
#include "module_loader.hpp"
#include "lua_module.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <csignal>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

using json = nlohmann::json;

volatile std::sig_atomic_t shutdown_requested = 0;

void signal_handler(int) {
    // POSIX signal handlers may only perform async-signal-safe work. The
    // watcher thread below translates this flag into a normal C++ shutdown.
    shutdown_requested = 1;
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "=== Discord Bot in C++ ===" << std::endl;
    std::cout << "Built with pure, organic C++\n" << std::endl;
    
    namespace fs = std::filesystem;
    std::error_code path_error;
    const fs::path launch_directory = fs::current_path(path_error);
    fs::path project_root = launch_directory;
    auto is_project_root = [](const fs::path& candidate) {
        std::error_code ec;
        return fs::is_directory(candidate / "modules", ec) &&
               fs::is_directory(candidate / "lib", ec);
    };
    if (!is_project_root(project_root)) {
        fs::path candidate = fs::absolute(argv[0], path_error).parent_path();
        for (int level = 0; level < 5 && !candidate.empty(); ++level) {
            if (is_project_root(candidate)) {
                project_root = candidate;
                break;
            }
            candidate = candidate.parent_path();
        }
    }
    if (!is_project_root(project_root)) {
        std::cerr << "Could not locate Routine project root (expected lib/ and "
                     "modules/ directories)." << std::endl;
        return 1;
    }
    fs::path config_path = argc > 1
        ? fs::absolute(argv[1], path_error)
        : project_root / "config.json";
    fs::current_path(project_root, path_error);
    if (path_error) {
        std::cerr << "Failed to enter project root: "
                  << path_error.message() << std::endl;
        return 1;
    }

    // Create bot instance
    auto bot = std::make_unique<discord::DiscordBot>();
    
    if (!bot->initialize(config_path.string())) {
        std::cerr << "Failed to initialize bot" << std::endl;
        return 1;
    }
    
    // Create command handler with "~" prefix
    auto command_handler = std::make_unique<discord::CommandHandler>("~");
    
    // Set bot reference for module command routing
    command_handler->set_bot(bot.get());
    
    // Create script engine
    auto script_engine = std::make_unique<discord::ScriptEngine>(
        bot.get(), 
        bot->get_module_loader()
    );
    
    // Register all commands
    discord::Commands::register_all(*command_handler, *bot);
    
    // Register script commands
    discord::Commands::register_script_commands(*command_handler, *bot, *script_engine);
    
    // Register event handlers
    bot->on_ready([](const std::string& /*data*/) {
        std::cout << "\nBot is fully connected and ready!" << std::endl;
        std::cout << "Use /help to see available commands "
                     "(legacy ~help also works)\n" << std::endl;
    });

    bot->on_modules_loaded([&](const std::string& /*data*/) {
        const json application_commands =
            command_handler->build_application_commands();
        if (!bot->sync_application_commands(
                application_commands.dump())) {
            std::cerr << "Application commands were not synchronized; "
                         "legacy prefix commands remain available."
                      << std::endl;
        }
    });

    bot->on_interaction_create([&](const std::string& data) {
        try {
            const json interaction = json::parse(data);
            command_handler->handle_interaction(interaction);
        } catch (const json::exception& error) {
            std::cerr << "Error parsing interaction: " << error.what()
                      << std::endl;
        }
    });
    
    bot->on_message_create([&](const std::string& data) {
        try {
            auto start = std::chrono::high_resolution_clock::now();
            
            json msg = json::parse(data);
            
            std::string content = msg.value("content", "");
            bool is_bot = msg.value("author", json::object()).value("bot", false);
            
            // Ignore bot messages
            if (is_bot) return;
            
            auto parse_end = std::chrono::high_resolution_clock::now();
            auto parse_time = std::chrono::duration_cast<std::chrono::microseconds>(parse_end - start);
            
            std::cout << "Message: \"" << content << "\" (parsed in " << parse_time.count() << "μs)" << std::endl;
            
            // Handle commands first
            std::string channel_id = msg.value("channel_id", "");
            std::string user_id = msg.value("author", json::object()).value("id", "");
            const std::string guild_id = msg.value("guild_id", "");
            if (!channel_id.empty() && !guild_id.empty()) {
                bot->remember_channel_guild(channel_id, guild_id);
            }
            if (!channel_id.empty() && !user_id.empty() &&
                msg.contains("member") && msg["member"].is_object() &&
                msg["member"].contains("roles") && msg["member"]["roles"].is_array()) {
                std::string roles;
                for (const auto& role : msg["member"]["roles"]) {
                    if (!role.is_string()) continue;
                    if (!roles.empty()) roles += ',';
                    roles += role.get<std::string>();
                }
                bot->remember_user_roles(channel_id, user_id, roles);
            }
            bot->get_module_loader()->on_message(channel_id, user_id, content);
            bot->get_lua_module_loader()->on_message(channel_id, user_id, content);

            bool handled = command_handler->handle_message(msg);
            
            // If not a command, let scripts handle it
            if (!handled) {
                script_engine->handle_event("message.create", msg);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            if (handled) {
                std::cout << "✓ Command handled in " << total_time.count() << "μs total" << std::endl;
            }
            
        } catch (const json::exception& e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
        }
    });
    
    bot->on_guild_create([&](const std::string& data) {
        try {
            json guild = json::parse(data);
            std::string guild_name = guild.value("name", "Unknown");
            std::string guild_id = guild.value("id", "");
            std::string owner_id = guild.value("owner_id", "");
            std::vector<std::pair<std::string, uint64_t>> role_permissions;
            if (guild.contains("roles") && guild["roles"].is_array()) {
                for (const auto& role : guild["roles"]) {
                    if (!role.is_object()) continue;
                    try {
                        role_permissions.emplace_back(
                            role.value("id", ""),
                            std::stoull(role.value("permissions", "0")));
                    } catch (...) {
                        // Ignore malformed permission payloads.
                    }
                }
            }
            bot->remember_guild_security(guild_id, owner_id, role_permissions);
            std::cout << "✓ Joined guild: " << guild_name << std::endl;
        } catch (const json::exception& e) {
            std::cerr << "Error parsing guild: " << e.what() << std::endl;
        }
    });

    const auto update_role_security = [&](const std::string& data) {
        try {
            const json event = json::parse(data);
            const std::string guild_id = event.value("guild_id", "");
            if (!event.contains("role") || !event["role"].is_object()) return;
            const json& role = event["role"];
            bot->remember_guild_role_security(
                guild_id, role.value("id", ""),
                std::stoull(role.value("permissions", "0")));
        } catch (const std::exception& e) {
            std::cerr << "Error updating guild role permissions: " << e.what() << std::endl;
        }
    };
    bot->on_guild_role_create(update_role_security);
    bot->on_guild_role_update(update_role_security);
    bot->on_guild_role_delete([&](const std::string& data) {
        try {
            const json event = json::parse(data);
            bot->forget_guild_role_security(
                event.value("guild_id", ""), event.value("role_id", ""));
        } catch (const json::exception& e) {
            std::cerr << "Error deleting cached guild role: " << e.what() << std::endl;
        }
    });
    
    // Connect to Discord
    if (!bot->connect()) {
        std::cerr << "Failed to connect to Discord" << std::endl;
        return 1;
    }

    std::atomic<bool> watcher_done{false};
    std::thread signal_watcher([&] {
        while (!watcher_done.load(std::memory_order_relaxed)) {
            if (shutdown_requested) {
                bot->request_stop();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    // Run the bot (blocking)
    bot->run();

    watcher_done.store(true, std::memory_order_relaxed);
    signal_watcher.join();
    bot->disconnect();

    return 0;
}
