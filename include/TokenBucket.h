#pragma once

#include<chrono>
#include<string>
#include<mutex>
#include <unordered_map>


// State container for each IP address
struct Bucket {
    double tokens;
    std::chrono::steady_clock::time_point last_access;
};


class TokenBucket {
private:
    double capacity;                                                    // tokens a bucket can hold
    double refill_rate;                                                 // tokens filled per second
    std::unordered_map<std::string, Bucket> store;                      // {IP address, Bucket}
    std::mutex mtx;                                                     // Ensures thread safety for concurrent requests

public:
    TokenBucket(double cap, double rate);
    bool allow_request(const std::string& client_ip);
};