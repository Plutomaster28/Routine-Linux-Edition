#include "rate_limiter.hpp"
#include <thread>
#include <iostream>

namespace discord {

RateLimiter::RateLimiter() {
}

RateLimiter::~RateLimiter() {
}

bool RateLimiter::can_make_request(const std::string& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = buckets_.find(route);
    if (it == buckets_.end()) {
        return true;  // No rate limit data yet
    }
    
    auto& bucket = it->second;
    auto now = std::chrono::steady_clock::now();
    
    // Check if we're past the reset time
    if (now >= bucket.reset_time) {
        bucket.remaining = bucket.limit;
        return true;
    }
    
    return bucket.remaining > 0;
}

void RateLimiter::update_from_headers(const std::string& route,
                                      int remaining,
                                      int limit,
                                      int reset_after,
                                      const std::string& bucket) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    BucketInfo info;
    info.remaining = remaining;
    info.limit = limit;
    info.reset_time = std::chrono::steady_clock::now() + 
                      std::chrono::milliseconds(reset_after);
    info.bucket_id = bucket;
    
    buckets_[route] = info;
}

void RateLimiter::wait_if_needed(const std::string& route) {
    int delay = get_delay(route);
    if (delay > 0) {
        std::cout << "Rate limited on route " << route 
                  << ", waiting " << delay << "ms" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    
    enforce_global_limit();
}

int RateLimiter::get_delay(const std::string& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = buckets_.find(route);
    if (it == buckets_.end()) {
        return 0;
    }
    
    auto& bucket = it->second;
    auto now = std::chrono::steady_clock::now();
    
    // If past reset time, no delay
    if (now >= bucket.reset_time) {
        return 0;
    }
    
    // If we have remaining requests, no delay
    if (bucket.remaining > 0) {
        bucket.remaining--;
        return 0;
    }
    
    // Calculate delay until reset
    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
        bucket.reset_time - now
    ).count();
    
    return static_cast<int>(delay);
}

void RateLimiter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    buckets_.clear();
    
    // Clear global request queue
    while (!global_requests_.empty()) {
        global_requests_.pop();
    }
}

void RateLimiter::enforce_global_limit() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto window_start = now - std::chrono::milliseconds(GLOBAL_WINDOW_MS);
    
    // Remove requests older than the window
    while (!global_requests_.empty() && global_requests_.front() < window_start) {
        global_requests_.pop();
    }
    
    // If we've hit the global limit, wait
    if (global_requests_.size() >= GLOBAL_LIMIT) {
        auto oldest = global_requests_.front();
        auto wait_until = oldest + std::chrono::milliseconds(GLOBAL_WINDOW_MS);
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
            wait_until - now
        );
        
        if (delay.count() > 0) {
            std::this_thread::sleep_for(delay);
        }
        
        global_requests_.pop();
    }
    
    // Record this request
    global_requests_.push(std::chrono::steady_clock::now());
}

} // namespace discord
