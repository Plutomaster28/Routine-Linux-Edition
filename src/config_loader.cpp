#include "config_loader.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

namespace discord {

ConfigLoader::ConfigLoader()
    : reconnect_attempts_(5), heartbeat_interval_(41250),
      gateway_intents_(1), slash_commands_enabled_(true),
      register_commands_on_start_(true) {
}

ConfigLoader::~ConfigLoader() {
}

bool ConfigLoader::load(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << config_path << std::endl;
        return false;
    }

    try {
        json config = json::parse(file);
        
        bot_token_ = config.value("bot_token", "");
        application_id_ = config.value("application_id", "");
        guild_id_ = config.value("guild_id", "");
        log_level_ = config.value("log_level", "info");
        reconnect_attempts_ = config.value("reconnect_attempts", 5);
        heartbeat_interval_ = config.value("heartbeat_interval", 41250);
        gateway_intents_ = config.value("gateway_intents", 1);
        if (config.contains("slash_commands") &&
            config["slash_commands"].is_object()) {
            const json& slash = config["slash_commands"];
            slash_commands_enabled_ = slash.value("enabled", true);
            register_commands_on_start_ =
                slash.value("register_on_start", true);
            slash_command_guild_id_ = slash.value("guild_id", "");
        } else {
            slash_commands_enabled_ = true;
            register_commands_on_start_ = true;
            slash_command_guild_id_ = guild_id_;
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }

    if (bot_token_.empty() || bot_token_ == "YOUR_BOT_TOKEN_HERE") {
        std::cerr << "Invalid bot token in config file!" << std::endl;
        return false;
    }
    constexpr int supported_intents = 3276799;
    if (gateway_intents_ <= 0 || (gateway_intents_ & ~supported_intents) != 0) {
        std::cerr << "Invalid gateway_intents in config file; unsupported Discord "
                     "intent bits were requested." << std::endl;
        return false;
    }
    if (slash_commands_enabled_ && application_id_.empty()) {
        std::cout << "application_id is empty; slash-command registration "
                     "will use the bot identity from Discord READY."
                  << std::endl;
    }

    std::cout << "Config loaded successfully" << std::endl;
    return true;
}

} // namespace discord
