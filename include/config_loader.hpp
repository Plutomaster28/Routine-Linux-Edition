#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include <string>
#include <fstream>
#include <stdexcept>

namespace discord {

class ConfigLoader {
public:
    ConfigLoader();
    ~ConfigLoader();

    bool load(const std::string& config_path);
    
    std::string get_bot_token() const { return bot_token_; }
    std::string get_application_id() const { return application_id_; }
    std::string get_guild_id() const { return guild_id_; }
    std::string get_log_level() const { return log_level_; }
    int get_reconnect_attempts() const { return reconnect_attempts_; }
    int get_heartbeat_interval() const { return heartbeat_interval_; }
    int get_gateway_intents() const { return gateway_intents_; }
    bool get_slash_commands_enabled() const { return slash_commands_enabled_; }
    bool get_register_commands_on_start() const {
        return register_commands_on_start_;
    }
    std::string get_slash_command_guild_id() const {
        return slash_command_guild_id_;
    }

private:
    std::string bot_token_;
    std::string application_id_;
    std::string guild_id_;
    std::string log_level_;
    int reconnect_attempts_;
    int heartbeat_interval_;
    int gateway_intents_;
    bool slash_commands_enabled_;
    bool register_commands_on_start_;
    std::string slash_command_guild_id_;
};

} // namespace discord

#endif // CONFIG_LOADER_HPP
