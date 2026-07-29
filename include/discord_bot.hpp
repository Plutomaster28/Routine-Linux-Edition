#ifndef DISCORD_BOT_HPP
#define DISCORD_BOT_HPP

#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>
#include "websocket_client.hpp"
#include "http_client.hpp"
#include "rate_limiter.hpp"
#include "event_dispatcher.hpp"
#include "config_loader.hpp"
#include "discord_types.hpp"

namespace discord {

// Forward declarations
class ModuleLoader;
class LuaModuleLoader;
class ExtensionLoader;

class DiscordBot {
public:
    DiscordBot();
    ~DiscordBot();

    // Initialize the bot with config
    bool initialize(const std::string& config_path);
    
    // Connect to Discord Gateway
    bool connect();
    
    // Disconnect from Discord
    void disconnect();
    
    // Start the bot (blocking)
    void run();
    void request_stop();
    
    // Register event handlers
    void on_message_create(EventHandler handler);
    void on_ready(EventHandler handler);
    void on_guild_create(EventHandler handler);
    void on_guild_role_create(EventHandler handler);
    void on_guild_role_update(EventHandler handler);
    void on_guild_role_delete(EventHandler handler);
    void on_interaction_create(EventHandler handler);
    void on_modules_loaded(EventHandler handler);
    
    // Send a message to a channel
    bool send_message(const std::string& channel_id, const std::string& content);

    // Application-command lifecycle. Interactions are deferred immediately;
    // send_message calls made by the synchronous handler are then routed to
    // the interaction response instead of creating unrelated channel posts.
    bool begin_interaction_response(const std::string& interaction_id,
                                    const std::string& interaction_token,
                                    const std::string& application_id,
                                    const std::string& channel_id);
    bool interaction_response_queued() const;
    void end_interaction_response();
    bool sync_application_commands(const std::string& command_payload);
    
    // Get channel info
    std::string get_channel(const std::string& channel_id);
    
    // Get bot uptime in seconds
    uint64_t get_uptime() const;
    bool is_connected() const;

    // Cache and resolve the guild for a Discord channel.
    void remember_channel_guild(const std::string& channel_id, const std::string& guild_id);
    std::string get_guild_id_for_channel(const std::string& channel_id) const;
    void remember_user_roles(const std::string& channel_id, const std::string& user_id,
                             const std::string& roles);
    std::string get_user_roles(const std::string& channel_id,
                               const std::string& user_id) const;
    void remember_guild_security(
        const std::string& guild_id, const std::string& owner_id,
        const std::vector<std::pair<std::string, uint64_t>>& role_permissions);
    void remember_guild_role_security(const std::string& guild_id,
                                      const std::string& role_id,
                                      uint64_t permissions);
    void forget_guild_role_security(const std::string& guild_id,
                                    const std::string& role_id);
    bool is_guild_admin(const std::string& channel_id, const std::string& user_id) const;
    
    // Module system access
    ModuleLoader* get_module_loader() const { return module_loader_.get(); }
    LuaModuleLoader* get_lua_module_loader() const { return lua_module_loader_.get(); }
    
    // Extension system access
    ExtensionLoader* get_extension_loader() const { return extension_loader_.get(); }

private:
    struct OutboundMessage {
        enum class Kind {
            ChannelMessage,
            InteractionOriginal,
            InteractionFollowup
        };

        Kind kind = Kind::ChannelMessage;
        std::string channel_id;
        std::string content;
        std::string application_id;
        std::string interaction_token;
    };

    void handle_gateway_message(const std::string& message);
    void handle_dispatch_event(const std::string& event_name, const std::string& data);
    void handle_hello(const std::string& data);
    void handle_heartbeat_ack();
    void send_identify();
    void send_heartbeat();
    void outbound_loop();
    void stop_outbound_worker();
    
    std::unique_ptr<WebSocketClient> ws_client_;
    std::unique_ptr<HttpClient> http_client_;
    std::unique_ptr<HttpClient> interaction_http_client_;
    std::unique_ptr<RateLimiter> rate_limiter_;
    std::unique_ptr<EventDispatcher> event_dispatcher_;
    std::unique_ptr<ConfigLoader> config_;
    std::unique_ptr<ExtensionLoader> extension_loader_;
    std::unique_ptr<ModuleLoader> module_loader_;
    std::unique_ptr<LuaModuleLoader> lua_module_loader_;
    
    std::atomic<bool> running_;
    std::atomic<bool> identified_;
    std::string session_id_;
    int sequence_number_;
    std::chrono::steady_clock::time_point start_time_;
    mutable std::mutex channel_guild_mutex_;
    std::unordered_map<std::string, std::string> channel_guilds_;
    std::unordered_map<std::string, std::string> user_roles_;
    std::unordered_map<std::string, std::string> guild_owners_;
    std::unordered_map<std::string, uint64_t> guild_role_permissions_;
    std::mutex application_command_mutex_;
    std::string application_id_;
    std::string pending_application_commands_;
    std::mutex outbound_mutex_;
    std::condition_variable outbound_cv_;
    std::queue<OutboundMessage> outbound_messages_;
    std::thread outbound_thread_;
    bool outbound_running_;
    
    static constexpr const char* GATEWAY_URL = "wss://gateway.discord.gg/?v=10&encoding=json";
    static constexpr const char* API_BASE_URL = "https://discord.com/api/v10";
};

} // namespace discord

#endif // DISCORD_BOT_HPP
