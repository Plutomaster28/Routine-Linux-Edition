#include "websocket_client.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdint>

namespace discord {

// libwebsockets callback
static int callback_discord(struct lws* wsi, enum lws_callback_reasons reason,
                            void* /*user*/, void* in, size_t len) {
    WebSocketClient* client = static_cast<WebSocketClient*>(
        lws_context_user(lws_get_context(wsi))
    );
    
    if (!client) return 0;
    
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "WebSocket connected" << std::endl;
            client->on_client_established(wsi);
            break;
            
        case LWS_CALLBACK_CLIENT_RECEIVE:
            client->on_receive(wsi, static_cast<const char*>(in), len);
            break;
            
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            client->on_writable(wsi);
            break;

        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
            client->on_peer_close(in, len);
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
            std::cout << "WebSocket closed" << std::endl;
            client->on_client_closed(wsi);
            break;
            
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "Connection error: " << (in ? (char*)in : "unknown") << std::endl;
            client->on_client_closed(wsi);
            break;
            
        default:
            break;
    }
    
    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        "",  // Discord doesn't use a WebSocket subprotocol
        callback_discord,
        0,
        65536,  // Larger rx buffer for Discord messages
        0, NULL, 0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 }
};

WebSocketClient::WebSocketClient()
    : connected_(false), should_heartbeat_(false), running_(false),
      reconnect_allowed_(true),
      reconnect_requested_(false),
      connection_attempt_complete_(false), context_(nullptr), wsi_(nullptr),
      sequence_number_(0) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect(const std::string& url) {
    gateway_url_ = url;
    connected_ = false;
    reconnect_allowed_ = true;
    reconnect_requested_ = false;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        connection_attempt_complete_ = false;
    }
    
    // Parse URL (wss://gateway.discord.gg/?v=10&encoding=json)
    std::string host = "gateway.discord.gg";
    std::string path = "/?v=10&encoding=json";
    int port = 443;
    
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.user = this;
    info.fd_limit_per_thread = 1 + 1 + 1;
    
    // Keep actionable transport diagnostics without libwebsockets' very noisy
    // internal INFO trace (including the harmless client-vhost "no cert" line).
    lws_set_log_level(LLL_ERR | LLL_WARN, nullptr);
    
    context_ = lws_create_context(&info);
    if (!context_) {
        std::cerr << "Failed to create libwebsockets context" << std::endl;
        return false;
    }
    
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    
    ccinfo.context = context_;
    ccinfo.address = host.c_str();
    ccinfo.port = port;
    ccinfo.path = path.c_str();
    ccinfo.host = host.c_str();
    ccinfo.origin = host.c_str();
    ccinfo.protocol = NULL;  // Discord doesn't require a specific protocol
    ccinfo.ssl_connection = LCCSCF_USE_SSL;
    ccinfo.pwsi = &wsi_;
    ccinfo.userdata = this;
    
    std::cout << "Attempting connection to wss://" << host << path << std::endl;
    
    wsi_ = lws_client_connect_via_info(&ccinfo);
    if (!wsi_) {
        std::cerr << "Failed to initiate connection to gateway" << std::endl;
        lws_context_destroy(context_);
        context_ = nullptr;
        return false;
    }
    
    running_ = true;
    service_thread_ = std::thread(&WebSocketClient::service_loop, this);

    std::unique_lock<std::mutex> lock(connection_mutex_);
    const bool completed = connection_cv_.wait_for(
        lock, std::chrono::seconds(15),
        [this] { return connection_attempt_complete_; });
    if (!completed || !connected_) {
        std::cerr << (completed
            ? "Gateway connection closed before startup completed"
            : "Timed out connecting to the Discord gateway") << std::endl;
        lock.unlock();
        disconnect();
        return false;
    }
    return true;
}

void WebSocketClient::disconnect() {
    if (!running_ && !context_ && !service_thread_.joinable() &&
        !heartbeat_thread_.joinable()) return;
    running_ = false;
    connected_ = false;
    stop_heartbeat();

    // libwebsockets owns and services the context on service_thread_. Wake it,
    // let that thread stop, and only then destroy the context. Destroying the
    // context while lws_service() is active is a use-after-free on Linux.
    if (context_) lws_cancel_service(context_);
    if (service_thread_.joinable()) {
        service_thread_.join();
    }

    if (context_) {
        lws_context_destroy(context_);
        context_ = nullptr;
    }
    wsi_ = nullptr;

    std::cout << "Disconnected from gateway" << std::endl;
}

bool WebSocketClient::send(const std::string& message) {
    if (!connected_) {
        std::cerr << "Cannot send: not connected" << std::endl;
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        send_queue_.push(message);
    }

    // lws_callback_on_writable() must run on the libwebsockets service thread.
    // Waking the service loop is the only cross-thread lws operation here.
    if (context_) lws_cancel_service(context_);
    
    return true;
}

void WebSocketClient::set_on_message(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_message_ = callback;
}

void WebSocketClient::set_on_error(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_error_ = callback;
}

void WebSocketClient::set_on_heartbeat(HeartbeatCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_heartbeat_ = callback;
}

bool WebSocketClient::is_connected() const {
    return connected_;
}

bool WebSocketClient::can_reconnect() const {
    return reconnect_allowed_;
}

void WebSocketClient::request_reconnect() {
    reconnect_requested_ = true;
    if (context_) lws_cancel_service(context_);
}

void WebSocketClient::start_heartbeat(int interval_ms) {
    stop_heartbeat();
    should_heartbeat_ = true;
    heartbeat_thread_ = std::thread(&WebSocketClient::heartbeat_loop, this, interval_ms);
}

void WebSocketClient::stop_heartbeat() {
    should_heartbeat_ = false;
    heartbeat_cv_.notify_all();
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

void WebSocketClient::on_client_established(struct lws* wsi) {
    wsi_ = wsi;
    connected_ = true;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        connection_attempt_complete_ = true;
    }
    connection_cv_.notify_all();
}

void WebSocketClient::on_client_closed(struct lws* wsi) {
    if (wsi_ == wsi) wsi_ = nullptr;
    connected_ = false;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        connection_attempt_complete_ = true;
    }
    connection_cv_.notify_all();
    should_heartbeat_ = false;
    heartbeat_cv_.notify_all();
}

void WebSocketClient::on_peer_close(const void* data, size_t len) {
    uint16_t code = 0;
    std::string reason;
    if (data && len >= 2) {
        const auto* bytes = static_cast<const unsigned char*>(data);
        code = static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
        if (len > 2) {
            reason.assign(reinterpret_cast<const char*>(bytes + 2), len - 2);
        }
    }
    std::cerr << "Discord gateway closed the websocket";
    if (code) std::cerr << " (code " << code << ')';
    if (!reason.empty()) std::cerr << ": " << reason;
    std::cerr << std::endl;
    if (code == 4013) {
        reconnect_allowed_ = false;
        std::cerr << "Gateway intents are invalid. Remove unsupported intent bits."
                  << std::endl;
    } else if (code == 4014) {
        reconnect_allowed_ = false;
        std::cerr << "A privileged gateway intent is disabled in the Discord "
                     "Developer Portal." << std::endl;
    } else if (code == 4004 || (code >= 4010 && code <= 4012)) {
        reconnect_allowed_ = false;
        std::cerr << "Discord reported a non-recoverable gateway configuration "
                     "error; automatic reconnect is disabled." << std::endl;
    }
}

void WebSocketClient::on_receive(struct lws* wsi, const char* data, size_t len) {
    if (data && len) receive_buffer_.append(data, len);
    if (!lws_is_final_fragment(wsi) || lws_remaining_packet_payload(wsi) != 0) {
        return;
    }
    std::string message;
    message.swap(receive_buffer_);
    MessageCallback callback;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback = on_message_;
    }
    if (callback) callback(message);
}

void WebSocketClient::on_writable(struct lws* wsi) {
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    if (send_queue_.empty()) {
        return;
    }
    
    std::string message = send_queue_.front();
    send_queue_.pop();
    
    // Prepare buffer with LWS_PRE padding
    size_t msg_len = message.length();
    std::vector<unsigned char> buf(LWS_PRE + msg_len);
    memcpy(buf.data() + LWS_PRE, message.c_str(), msg_len);
    
    int written = lws_write(wsi, buf.data() + LWS_PRE, msg_len, LWS_WRITE_TEXT);
    
    if (written < 0) {
        std::cerr << "Error writing to websocket" << std::endl;
    }
    
    if (!send_queue_.empty()) lws_callback_on_writable(wsi);
}

void WebSocketClient::service_loop() {
    while (running_ && context_) {
        lws_service(context_, 50);
        bool pending = false;
        {
            std::lock_guard<std::mutex> lock(send_mutex_);
            pending = !send_queue_.empty();
        }
        if (pending && connected_ && wsi_) {
            lws_callback_on_writable(wsi_);
        }
        if (reconnect_requested_.exchange(false) && wsi_) {
            lws_set_timeout(
                wsi_, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);
        }
    }
}

void WebSocketClient::heartbeat_loop(int interval_ms) {
    while (should_heartbeat_ && connected_) {
        std::unique_lock<std::mutex> heartbeat_lock(heartbeat_mutex_);
        const bool stopping = heartbeat_cv_.wait_for(
            heartbeat_lock, std::chrono::milliseconds(interval_ms),
            [this] { return !should_heartbeat_ || !connected_; });
        if (stopping) break;
        heartbeat_lock.unlock();

        HeartbeatCallback callback;
        {
            std::lock_guard<std::mutex> callback_lock(callback_mutex_);
            callback = on_heartbeat_;
        }
        if (callback) callback();
    }
}

} // namespace discord
