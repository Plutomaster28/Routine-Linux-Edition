#include "discord_bot.hpp"
#include "discord_message_utils.hpp"
#include "module_loader.hpp"
#include "lua_module.hpp"
#include "extension_loader.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>
#include <thread>

using json = nlohmann::json;

namespace discord {
namespace {

struct ActiveInteraction {
    DiscordBot* bot = nullptr;
    std::string application_id;
    std::string token;
    std::string channel_id;
    bool original_response_queued = false;
};

thread_local std::optional<ActiveInteraction> active_interaction;

} // namespace

DiscordBot::DiscordBot()
    : running_(false), identified_(false), sequence_number_(0),
      start_time_(std::chrono::steady_clock::now()), outbound_running_(true) {
    
    ws_client_ = std::make_unique<WebSocketClient>();
    http_client_ = std::make_unique<HttpClient>();
    interaction_http_client_ = std::make_unique<HttpClient>();
    rate_limiter_ = std::make_unique<RateLimiter>();
    event_dispatcher_ = std::make_unique<EventDispatcher>();
    config_ = std::make_unique<ConfigLoader>();
    extension_loader_ = std::make_unique<ExtensionLoader>(this);
    module_loader_ = std::make_unique<ModuleLoader>(this);
    lua_module_loader_ = std::make_unique<LuaModuleLoader>(this);
    outbound_thread_ = std::thread(&DiscordBot::outbound_loop, this);
}

DiscordBot::~DiscordBot() {
    disconnect();
    // Modules may use the send-message bridge during shutdown, so unload them
    // before stopping the owned REST worker.
    lua_module_loader_.reset();
    module_loader_.reset();
    extension_loader_.reset();
    stop_outbound_worker();
}

bool DiscordBot::initialize(const std::string& config_path) {
    if (!config_->load(config_path)) {
        std::cerr << "Failed to load config" << std::endl;
        return false;
    }
    
    // Set up HTTP client
    http_client_->set_base_url(API_BASE_URL);
    http_client_->set_authorization(config_->get_bot_token());
    interaction_http_client_->set_base_url(API_BASE_URL);
    interaction_http_client_->set_authorization(config_->get_bot_token());
    {
        std::lock_guard<std::mutex> lock(application_command_mutex_);
        application_id_ = config_->get_application_id();
    }
    
    std::cout << "Bot initialized successfully" << std::endl;
    return true;
}

bool DiscordBot::connect() {
    // Set up WebSocket callbacks BEFORE connecting
    ws_client_->set_on_message([this](const std::string& msg) {
        handle_gateway_message(msg);
    });
    
    ws_client_->set_on_error([](const std::string& error) {
        std::cerr << "WebSocket error: " << error << std::endl;
    });
    
    ws_client_->set_on_heartbeat([this]() {
        send_heartbeat();
    });
    
    if (!ws_client_->connect(GATEWAY_URL)) {
        std::cerr << "Failed to connect to gateway" << std::endl;
        return false;
    }
    
    std::cout << "Connecting to Discord gateway..." << std::endl;
    return true;
}

void DiscordBot::disconnect() {
    running_ = false;
    if (ws_client_) {
        ws_client_->disconnect();
    }
}

void DiscordBot::request_stop() {
    running_ = false;
}

void DiscordBot::run() {
    running_ = true;
    
    // Load extensions first (they provide capabilities for modules)
    std::cout << "\n=== Loading Extensions ===" << std::endl;
    
    std::vector<std::string> ext_paths = {
        "lib",              // Current directory
        "../lib",           // Parent directory (for build/)
        "../../lib"         // Two levels up
    };
    
    size_t ext_loaded = 0;
    bool found_ext_dir = false;
    
    for (const auto& path : ext_paths) {
        std::cout << "Trying: " << path << "..." << std::endl;
        
        size_t count = extension_loader_->load_extensions_from_directory(path);
        
        if (count > 0) {
            std::cout << "✓ Found extension directory: " << path << std::endl;
            ext_loaded += count;
            found_ext_dir = true;
            break;
        }
    }
    
    if (!found_ext_dir) {
        std::cout << "⚠ No extension directory found (this is optional)" << std::endl;
    } else {
        std::cout << "\n✓ Loaded " << ext_loaded << " extension(s)\n" << std::endl;
    }
    
    // Load modules from modules directory
    std::cout << "\n=== Loading Modules ===" << std::endl;
    
    // Try multiple possible module paths
    std::vector<std::string> module_paths = {
        "modules",           // Current directory
        "../modules",        // Parent directory (for build/)
        "../../modules"      // Two levels up
    };
    
    size_t loaded = 0;
    size_t lua_loaded = 0;
    bool found_module_dir = false;
    
    for (const auto& path : module_paths) {
        std::cout << "Trying: " << path << "..." << std::endl;
        
        size_t native = module_loader_->load_modules_from_directory(path);
        size_t lua = lua_module_loader_->load_modules_from_directory(path);
        
        if (native > 0 || lua > 0) {
            std::cout << "✓ Found module directory: " << path << std::endl;
            loaded += native;
            lua_loaded += lua;
            found_module_dir = true;
            break;  // Stop after finding modules
        }
    }
    
    if (!found_module_dir) {
        std::cout << "⚠ No module directory found (checked: modules, ../modules, ../../modules)" << std::endl;
        std::cout << "  Place modules in a 'modules/' folder relative to the executable" << std::endl;
    }
    
    std::cout << "\n✓ Loaded " << loaded << " native module(s) and " 
              << lua_loaded << " Lua module(s)\n" << std::endl;

    // Internal lifecycle event used by the command system to synchronize the
    // final command set only after every dynamic module has registered.
    event_dispatcher_->dispatch("ROUTINE_MODULES_LOADED", "{}");
    
    std::cout << "Bot is now running. Press Ctrl+C to stop." << std::endl;
    
    auto last_activity = std::chrono::steady_clock::now();
    auto last_module_tick = std::chrono::steady_clock::now();
    
    // Main loop with connection monitoring
    while (running_) {
        if (!ws_client_->is_connected()) {
            if (!ws_client_->can_reconnect()) break;
            std::cerr << "\nLost connection to Discord - attempting reconnect..."
                      << std::endl;
            bool reconnected = false;
            const int attempts = std::max(0, config_->get_reconnect_attempts());
            for (int attempt = 1; running_ && attempt <= attempts; ++attempt) {
                ws_client_->disconnect();
                const int delay_seconds = std::min(30, attempt * 2);
                std::cerr << "Reconnect attempt " << attempt << '/' << attempts
                          << " in " << delay_seconds << "s" << std::endl;
                for (int tenth = 0; running_ && tenth < delay_seconds * 10; ++tenth) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                if (running_ && ws_client_->connect(GATEWAY_URL)) {
                    reconnected = true;
                    std::cout << "Reconnected to Discord gateway." << std::endl;
                    break;
                }
            }
            if (!reconnected) break;
        }
        
        // Check if we've received any activity recently (within 2 minutes)
        auto now = std::chrono::steady_clock::now();
        if (now - last_module_tick >= std::chrono::seconds(1)) {
            module_loader_->on_tick();
            lua_module_loader_->on_tick();
            last_module_tick = now;
        }
        auto idle_time = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity).count();
        
        if (idle_time > 120) {
            std::cout << "\nNo activity for " << idle_time << " seconds - connection may be stale" << std::endl;
            last_activity = now; // Reset to avoid spam
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!ws_client_->is_connected()) {
        std::cerr << "Connection lost." << std::endl;
    }
}

void DiscordBot::on_message_create(EventHandler handler) {
    event_dispatcher_->on("MESSAGE_CREATE", handler);
}

void DiscordBot::on_ready(EventHandler handler) {
    event_dispatcher_->on("READY", handler);
}

void DiscordBot::on_guild_create(EventHandler handler) {
    event_dispatcher_->on("GUILD_CREATE", handler);
}

void DiscordBot::on_guild_role_create(EventHandler handler) {
    event_dispatcher_->on("GUILD_ROLE_CREATE", handler);
}

void DiscordBot::on_guild_role_update(EventHandler handler) {
    event_dispatcher_->on("GUILD_ROLE_UPDATE", handler);
}

void DiscordBot::on_guild_role_delete(EventHandler handler) {
    event_dispatcher_->on("GUILD_ROLE_DELETE", handler);
}

void DiscordBot::on_interaction_create(EventHandler handler) {
    event_dispatcher_->on("INTERACTION_CREATE", handler);
}

void DiscordBot::on_modules_loaded(EventHandler handler) {
    event_dispatcher_->on("ROUTINE_MODULES_LOADED", handler);
}

bool DiscordBot::begin_interaction_response(
    const std::string& interaction_id,
    const std::string& interaction_token,
    const std::string& application_id,
    const std::string& channel_id) {
    if (interaction_id.empty() || interaction_token.empty() ||
        application_id.empty() || channel_id.empty() ||
        !interaction_http_client_) {
        return false;
    }

    const std::string route =
        "/interactions/" + interaction_id + "/" + interaction_token +
        "/callback";
    const json payload = {
        {"type", 5},
        {"data", json::object()}
    };
    const HttpResponse response =
        interaction_http_client_->post(route, payload.dump());
    if (!response.success) {
        std::cerr << "✗ Failed to defer Discord interaction (HTTP "
                  << response.status_code << "): " << response.body
                  << std::endl;
        return false;
    }

    active_interaction = ActiveInteraction{
        this, application_id, interaction_token, channel_id, false
    };
    return true;
}

void DiscordBot::end_interaction_response() {
    if (active_interaction && active_interaction->bot == this) {
        active_interaction.reset();
    }
}

bool DiscordBot::interaction_response_queued() const {
    return active_interaction && active_interaction->bot == this &&
           active_interaction->original_response_queued;
}

bool DiscordBot::sync_application_commands(
    const std::string& command_payload) {
    if (!config_->get_slash_commands_enabled() ||
        !config_->get_register_commands_on_start()) {
        std::cout << "Application-command registration is disabled."
                  << std::endl;
        return true;
    }
    std::string application_id;
    {
        std::lock_guard<std::mutex> lock(application_command_mutex_);
        application_id = application_id_;
        if (application_id.empty()) {
            pending_application_commands_ = command_payload;
            std::cout << "Application-command registration is waiting for "
                         "Discord READY identity." << std::endl;
            return true;
        }
    }

    std::string route = "/applications/" + application_id;
    const std::string guild_id = config_->get_slash_command_guild_id();
    if (guild_id.empty()) {
        route += "/commands";
    } else {
        route += "/guilds/" + guild_id + "/commands";
    }

    const HttpResponse response = http_client_->put(route, command_payload);
    if (!response.success) {
        std::cerr << "✗ Failed to synchronize application commands (HTTP "
                  << response.status_code << "): " << response.body
                  << std::endl;
        return false;
    }

    try {
        const json registered = json::parse(response.body);
        std::cout << "✓ Synchronized " << registered.size()
                  << (guild_id.empty() ? " global" : " guild")
                  << " application command(s)" << std::endl;
    } catch (const json::exception&) {
        std::cout << "✓ Synchronized application commands" << std::endl;
    }
    return true;
}

bool DiscordBot::send_message(const std::string& channel_id, const std::string& content) {
    if (channel_id.empty() || content.empty()) return false;

    const std::vector<std::string> chunks = split_discord_message(content);
    if (chunks.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(outbound_mutex_);
        constexpr std::size_t maximum_queued_messages = 1000;
        if (!outbound_running_ ||
            chunks.size() > maximum_queued_messages - std::min(
                outbound_messages_.size(), maximum_queued_messages)) {
            return false;
        }
        for (const std::string& chunk : chunks) {
            OutboundMessage message;
            message.channel_id = channel_id;
            message.content = chunk;
            if (active_interaction &&
                active_interaction->bot == this &&
                active_interaction->channel_id == channel_id) {
                message.application_id =
                    active_interaction->application_id;
                message.interaction_token = active_interaction->token;
                if (!active_interaction->original_response_queued) {
                    message.kind =
                        OutboundMessage::Kind::InteractionOriginal;
                    active_interaction->original_response_queued = true;
                } else {
                    message.kind =
                        OutboundMessage::Kind::InteractionFollowup;
                }
            }
            outbound_messages_.push(std::move(message));
        }
    }
    outbound_cv_.notify_one();
    return true;
}

void DiscordBot::outbound_loop() {
    for (;;) {
        OutboundMessage message;
        {
            std::unique_lock<std::mutex> lock(outbound_mutex_);
            outbound_cv_.wait(lock, [this] {
                return !outbound_running_ || !outbound_messages_.empty();
            });
            if (!outbound_running_ && outbound_messages_.empty()) break;
            message = std::move(outbound_messages_.front());
            outbound_messages_.pop();
        }

        std::string route;
        const char* action = "Sending message";
        if (message.kind == OutboundMessage::Kind::ChannelMessage) {
            route = "/channels/" + message.channel_id + "/messages";
            rate_limiter_->wait_if_needed(route);
        } else if (message.kind ==
                   OutboundMessage::Kind::InteractionOriginal) {
            route = "/webhooks/" + message.application_id + "/" +
                    message.interaction_token + "/messages/@original";
            action = "Editing interaction response";
        } else {
            route = "/webhooks/" + message.application_id + "/" +
                    message.interaction_token;
            action = "Sending interaction follow-up";
        }
        const json payload = {{"content", message.content}};
        std::cout << "→ " << action << ": \""
                  << message.content.substr(0, 100)
                  << (message.content.length() > 100 ? "..." : "") << "\""
                  << std::endl;
        const auto response =
            message.kind == OutboundMessage::Kind::InteractionOriginal
                ? http_client_->patch(route, payload.dump())
                : http_client_->post(route, payload.dump());

        try {
            if (message.kind == OutboundMessage::Kind::ChannelMessage &&
                response.headers.count("x-ratelimit-remaining")) {
                const int remaining =
                    std::stoi(response.headers.at("x-ratelimit-remaining"));
                const int limit = response.headers.count("x-ratelimit-limit")
                    ? std::stoi(response.headers.at("x-ratelimit-limit")) : 5;
                const int reset_after_ms =
                    response.headers.count("x-ratelimit-reset-after")
                    ? static_cast<int>(std::stod(
                        response.headers.at("x-ratelimit-reset-after")) * 1000.0)
                    : 0;
                const std::string bucket =
                    response.headers.count("x-ratelimit-bucket")
                    ? response.headers.at("x-ratelimit-bucket") : "";
                rate_limiter_->update_from_headers(
                    route, remaining, limit, reset_after_ms, bucket);
            }
        } catch (const std::exception& error) {
            std::cerr << "Invalid Discord rate-limit headers: "
                      << error.what() << std::endl;
        }

        if (!response.success) {
            std::cerr << "✗ Failed to send message (HTTP "
                      << response.status_code << "): " << response.body << std::endl;
        } else {
            std::cout << "✓ Message sent successfully" << std::endl;
        }
    }
}

void DiscordBot::stop_outbound_worker() {
    {
        std::lock_guard<std::mutex> lock(outbound_mutex_);
        outbound_running_ = false;
    }
    outbound_cv_.notify_all();
    if (outbound_thread_.joinable()) outbound_thread_.join();
}

std::string DiscordBot::get_channel(const std::string& channel_id) {
    std::string route = "/channels/" + channel_id;
    rate_limiter_->wait_if_needed(route);
    
    auto response = http_client_->get(route);
    
    if (response.success) {
        return response.body;
    }
    
    return "";
}

uint64_t DiscordBot::get_uptime() const {
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return static_cast<uint64_t>(uptime.count());
}

bool DiscordBot::is_connected() const {
    return ws_client_ && ws_client_->is_connected();
}

void DiscordBot::remember_channel_guild(const std::string& channel_id,
                                        const std::string& guild_id) {
    if (channel_id.empty() || guild_id.empty()) return;
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    channel_guilds_[channel_id] = guild_id;
}

std::string DiscordBot::get_guild_id_for_channel(const std::string& channel_id) const {
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    auto it = channel_guilds_.find(channel_id);
    return it == channel_guilds_.end() ? std::string() : it->second;
}

void DiscordBot::remember_user_roles(const std::string& channel_id,
                                     const std::string& user_id,
                                     const std::string& roles) {
    if (channel_id.empty() || user_id.empty()) return;
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    user_roles_[channel_id + '\x1f' + user_id] = roles;
}

std::string DiscordBot::get_user_roles(const std::string& channel_id,
                                       const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    auto it = user_roles_.find(channel_id + '\x1f' + user_id);
    return it == user_roles_.end() ? std::string() : it->second;
}

void DiscordBot::remember_guild_security(
    const std::string& guild_id, const std::string& owner_id,
    const std::vector<std::pair<std::string, uint64_t>>& role_permissions) {
    if (guild_id.empty()) return;
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    guild_owners_[guild_id] = owner_id;
    const std::string prefix = guild_id + '\x1f';
    for (auto it = guild_role_permissions_.begin(); it != guild_role_permissions_.end();) {
        if (it->first.compare(0, prefix.size(), prefix) == 0) {
            it = guild_role_permissions_.erase(it);
        } else {
            ++it;
        }
    }
    for (const auto& role : role_permissions) {
        guild_role_permissions_[guild_id + '\x1f' + role.first] = role.second;
    }
}

void DiscordBot::remember_guild_role_security(const std::string& guild_id,
                                              const std::string& role_id,
                                              uint64_t permissions) {
    if (guild_id.empty() || role_id.empty()) return;
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    guild_role_permissions_[guild_id + '\x1f' + role_id] = permissions;
}

void DiscordBot::forget_guild_role_security(const std::string& guild_id,
                                            const std::string& role_id) {
    if (guild_id.empty() || role_id.empty()) return;
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    guild_role_permissions_.erase(guild_id + '\x1f' + role_id);
}

bool DiscordBot::is_guild_admin(const std::string& channel_id,
                                const std::string& user_id) const {
    constexpr uint64_t administrator = 1ULL << 3;
    constexpr uint64_t manage_guild = 1ULL << 5;
    std::lock_guard<std::mutex> lock(channel_guild_mutex_);
    auto channel = channel_guilds_.find(channel_id);
    if (channel == channel_guilds_.end()) return false;
    const std::string& guild_id = channel->second;
    auto owner = guild_owners_.find(guild_id);
    if (owner != guild_owners_.end() && owner->second == user_id) return true;
    auto everyone = guild_role_permissions_.find(guild_id + '\x1f' + guild_id);
    if (everyone != guild_role_permissions_.end() &&
        (everyone->second & (administrator | manage_guild))) {
        return true;
    }
    auto roles = user_roles_.find(channel_id + '\x1f' + user_id);
    if (roles == user_roles_.end()) return false;
    std::istringstream input(roles->second);
    std::string role_id;
    while (std::getline(input, role_id, ',')) {
        auto permissions = guild_role_permissions_.find(guild_id + '\x1f' + role_id);
        if (permissions != guild_role_permissions_.end() &&
            (permissions->second & (administrator | manage_guild))) {
            return true;
        }
    }
    return false;
}

void DiscordBot::handle_gateway_message(const std::string& message) {
    try {
        json payload = json::parse(message);
        
        int op = payload["op"];
        
        // Update sequence number if present
        if (!payload["s"].is_null()) {
            sequence_number_ = payload["s"];
        }
        
        switch (op) {
            case 10: // HELLO
                handle_hello(payload["d"].dump());
                break;
                
            case 0: // DISPATCH
                {
                    std::string event_type = payload["t"];
                    std::string data = payload["d"].dump();
                    handle_dispatch_event(event_type, data);
                }
                break;
                
            case 11: // HEARTBEAT_ACK
                handle_heartbeat_ack();
                break;
                
            case 1: // HEARTBEAT (server requesting)
                send_heartbeat();
                break;
                
            case 9: // INVALID_SESSION
                std::cerr << "Invalid session, reconnecting..." << std::endl;
                ws_client_->request_reconnect();
                break;
                
            case 7: // RECONNECT
                std::cerr << "Server requested reconnect" << std::endl;
                ws_client_->request_reconnect();
                break;
                
            default:
                std::cout << "Unknown opcode: " << op << std::endl;
                break;
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error in gateway message: " << e.what() << std::endl;
        std::cerr << "Message: " << message << std::endl;
    }
}

void DiscordBot::handle_dispatch_event(const std::string& event_name, const std::string& data) {
    std::string pending_application_commands;
    if (event_name == "READY") {
        identified_ = true;
        try {
            json ready_data = json::parse(data);
            session_id_ = ready_data["session_id"];
            std::cout << "✓ Bot is READY! Session ID: " << session_id_ << std::endl;
            
            if (ready_data.contains("user")) {
                std::string username = ready_data["user"]["username"];
                std::cout << "✓ Logged in as: " << username << std::endl;
                const std::string ready_application_id =
                    ready_data["user"].value("id", "");
                std::lock_guard<std::mutex> lock(
                    application_command_mutex_);
                if (application_id_.empty()) {
                    application_id_ = ready_application_id;
                }
                if (!application_id_.empty() &&
                    !pending_application_commands_.empty()) {
                    pending_application_commands =
                        std::move(pending_application_commands_);
                    pending_application_commands_.clear();
                }
            }
        } catch (const json::exception& e) {
            std::cerr << "Error parsing READY data: " << e.what() << std::endl;
        }
    }
    
    // Dispatch to registered event handlers
    event_dispatcher_->dispatch(event_name, data);
    if (!pending_application_commands.empty()) {
        sync_application_commands(pending_application_commands);
    }
}

void DiscordBot::handle_hello(const std::string& data) {
    try {
        json hello_data = json::parse(data);
        int heartbeat_interval = hello_data["heartbeat_interval"];
        
        std::cout << "✓ Received HELLO, heartbeat interval: " 
                  << heartbeat_interval << "ms" << std::endl;
        
        // Start heartbeat
        ws_client_->start_heartbeat(heartbeat_interval);
        
        // Send IDENTIFY
        send_identify();
        
    } catch (const json::exception& e) {
        std::cerr << "Error parsing HELLO data: " << e.what() << std::endl;
    }
}

void DiscordBot::handle_heartbeat_ack() {
    // Heartbeat acknowledged - connection is healthy
    static int ack_count = 0;
    ack_count++;
    if (ack_count % 10 == 0) {
        std::cout << "Connection healthy (" << ack_count << " heartbeats)" << std::endl;
    }
}

void DiscordBot::send_identify() {
    const int intents = config_->get_gateway_intents();
    
    json identify = {
        {"op", 2},
        {"d", {
            {"token", config_->get_bot_token()},
            {"intents", intents},
            {"properties", {
                {"os", "linux"},
                {"browser", "discord-cpp-bot"},
                {"device", "discord-cpp-bot"}
            }}
        }}
    };
    
    std::cout << "→ Sending IDENTIFY to Discord..." << std::endl;
    ws_client_->send(identify.dump());
}

void DiscordBot::send_heartbeat() {
    json heartbeat = {
        {"op", 1},
        {"d", sequence_number_ > 0 ? json(sequence_number_) : json(nullptr)}
    };
    
    static int beat_count = 0;
    beat_count++;
    if (beat_count % 10 == 0) {
        std::cout << "Sent " << beat_count << " heartbeats, seq: " << sequence_number_ << std::endl;
    }
    
    ws_client_->send(heartbeat.dump());
}

} // namespace discord
