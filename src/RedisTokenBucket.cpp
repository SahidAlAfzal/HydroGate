#include "RedisTokenBucket.h"
#include <chrono>
#include <vector>
#include <string>


RedisTokenBucket::RedisTokenBucket(const std::string& redis_url, double cap, double rate) : redis(redis_url), capacity(cap), refill_rate(rate) {}


bool RedisTokenBucket::allow_request(const std::string& client_ip) {
    // Get current time in milliseconds
    auto now = std::chrono::steady_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();


    std::vector<std::optional<std::string>> state;
    redis.hmget(client_ip, {"tokens", "last_access"}, std::back_inserter(state));

    double current_tokens = capacity - 1.0;
    long long last_access = now_ms;

    bool allowed = true;

    // If the IP exists in Redis, calculate the refill
    if(state[0] && state[1]) {
        current_tokens = std::stod(*state[0]);
        last_access = std::stoll(*state[1]);


        double elapsed_seconds = (now_ms - last_access) / 1000.0;
        current_tokens += elapsed_seconds * refill_rate;
        // Calculate elapsed time in seconds

        if(current_tokens > capacity) {
            current_tokens = capacity;
        }


        if(current_tokens >= 1.0) {
            current_tokens -= 1.0;
            last_access = now_ms;

        } else {
            allowed = false;
        }
    }


    // Only write back to Redis if the request was accepted or it's a new IP
    if(allowed) {
        redis.hset(client_ip, "tokens", std::to_string(current_tokens));
        redis.hset(client_ip, "last_access", std::to_string(last_access));

        // TTL (Time To Live): Expire the key after 60 seconds of inactivity 
        // This is crucial so your Redis memory doesn't fill up with stale IP addresses.
        redis.expire(client_ip, 60);
    }

    return allowed;
}