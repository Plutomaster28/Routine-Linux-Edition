#ifndef EVENT_DISPATCHER_HPP
#define EVENT_DISPATCHER_HPP

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <mutex>
#include "discord_types.hpp"

namespace discord {

class EventDispatcher {
public:
    EventDispatcher();
    ~EventDispatcher();

    // Register event handlers
    void on(const std::string& event_name, EventHandler handler);
    
    // Dispatch an event to all registered handlers
    void dispatch(const std::string& event_name, const std::string& data);
    
    // Remove all handlers for an event
    void remove_handlers(const std::string& event_name);
    
    // Clear all handlers
    void clear();

private:
    std::map<std::string, std::vector<EventHandler>> handlers_;
    std::mutex mutex_;
};

} // namespace discord

#endif // EVENT_DISPATCHER_HPP
