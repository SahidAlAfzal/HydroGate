#pragma once

#include <string>
#include <sw/redis++/redis++.h>

class RedisTokenBucket {
private:
    double capacity;
    double refill_rate;
    sw::redis::Redis redis;     // redis client


public:
    RedisTokenBucket(const std::string& redis_url, double cap, double rate);
    bool allow_request(const std::string& client_ip);
}; 