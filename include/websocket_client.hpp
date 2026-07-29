#ifndef WEBSOCKET_CLIENT_HPP
#define WEBSOCKET_CLIENT_HPP

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <libwebsockets.h>
#include "discord_types.hpp"

namespace discord {

class WebSocketClient {
public:
    using MessageCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using HeartbeatCallback = std::function<void()>;

    WebSocketClient();
    ~WebSocketClient();

    bool connect(const std::string& url);
    void disconnect();
    bool send(const std::string& message);
    
    void set_on_message(MessageCallback callback);
    void set_on_error(ErrorCallback callback);
    void set_on_heartbeat(HeartbeatCallback callback);
    
    bool is_connected() const;
    bool can_reconnect() const;
    void request_reconnect();
    void start_heartbeat(int interval_ms);
    void stop_heartbeat();

    // Called by libwebsockets callback
    void on_writable(struct lws* wsi);
    void on_receive(struct lws* wsi, const char* data, size_t len);
    void on_client_established(struct lws* wsi);
    void on_client_closed(struct lws* wsi);
    void on_peer_close(const void* data, size_t len);

private:
    void service_loop();
    void heartbeat_loop(int interval_ms);
    
    std::atomic<bool> connected_;
    std::atomic<bool> should_heartbeat_;
    std::atomic<bool> running_;
    std::atomic<bool> reconnect_allowed_;
    std::atomic<bool> reconnect_requested_;
    std::thread service_thread_;
    std::thread heartbeat_thread_;
    std::mutex connection_mutex_;
    std::condition_variable connection_cv_;
    bool connection_attempt_complete_;
    std::mutex heartbeat_mutex_;
    std::condition_variable heartbeat_cv_;
    
    MessageCallback on_message_;
    ErrorCallback on_error_;
    HeartbeatCallback on_heartbeat_;
    
    // libwebsockets context and connection
    struct lws_context* context_;
    struct lws* wsi_;
    
    std::string gateway_url_;
    std::string receive_buffer_;
    int sequence_number_;
    
    // Thread-safe message queue for sending
    std::queue<std::string> send_queue_;
    std::mutex send_mutex_;
    std::mutex callback_mutex_;
};

} // namespace discord

#endif // WEBSOCKET_CLIENT_HPP
