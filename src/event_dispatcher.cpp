#include "event_dispatcher.hpp"
#include <iostream>

namespace discord {

EventDispatcher::EventDispatcher() {
}

EventDispatcher::~EventDispatcher() {
}

void EventDispatcher::on(const std::string& event_name, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[event_name].push_back(handler);
}

void EventDispatcher::dispatch(const std::string& event_name, const std::string& data) {
    std::vector<EventHandler> handlers_copy;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(event_name);
        if (it != handlers_.end()) {
            handlers_copy = it->second;
        }
    }
    
    // Execute handlers outside the lock to prevent deadlocks
    for (const auto& handler : handlers_copy) {
        try {
            handler(data);
        } catch (const std::exception& e) {
            std::cerr << "Exception in event handler for " << event_name 
                      << ": " << e.what() << std::endl;
        }
    }
}

void EventDispatcher::remove_handlers(const std::string& event_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(event_name);
}

void EventDispatcher::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.clear();
}

} // namespace discord
