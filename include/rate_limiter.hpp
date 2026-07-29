#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

#include <string>
#include <map>
#include <chrono>
#include <mutex>
#include <queue>
#include <functional>
#include "discord_types.hpp"

namespace discord {

class RateLimiter {
public:
    RateLimiter();
    ~RateLimiter();

    // Check if a request to a specific route can be made
    bool can_make_request(const std::string& route);
    
    // Update rate limit info from response headers
    void update_from_headers(const std::string& route,
                            int remaining,
                            int limit,
                            int reset_after,
                            const std::string& bucket);
    
    // Wait if necessary before making a request
    void wait_if_needed(const std::string& route);
    
    // Get the delay until next request is allowed (in ms)
    int get_delay(const std::string& route);
    
    // Reset all rate limit data
    void reset();

private:
    struct BucketInfo {
        int remaining;
        int limit;
        std::chrono::steady_clock::time_point reset_time;
        std::string bucket_id;
    };

    std::map<std::string, BucketInfo> buckets_;
    std::mutex mutex_;
    
    // Global rate limit (50 requests per second across all routes)
    static constexpr int GLOBAL_LIMIT = 50;
    static constexpr int GLOBAL_WINDOW_MS = 1000;
    
    std::queue<std::chrono::steady_clock::time_point> global_requests_;
    void enforce_global_limit();
};

} // namespace discord

#endif // RATE_LIMITER_HPP
