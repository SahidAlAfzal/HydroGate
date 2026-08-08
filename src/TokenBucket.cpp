#include "TokenBucket.h"

#include <string>

TokenBucket::TokenBucket(double cap, double rate) {
    capacity = cap;
    refill_rate = rate;
}


bool TokenBucket::allow_request(const std::string& client_ip) {
    // Lock the map to prevent race conditions from concurrent network requests
    std::lock_guard<std::mutex> lock(mtx);

    auto now = std::chrono::steady_clock::now();


    if(store.find(client_ip) == store.end()) {    // Client_id asking its first request
        store[client_ip] = {capacity - 1.0, now};
        return true;
    }


    Bucket& bucket = store[client_ip];


    // Calculate the exact time elapsed in seconds (fractional)
    std::chrono::duration<double> elasped_seconds = now - bucket.last_access;


    // calculating current tokens in the bucket
    bucket.tokens += elasped_seconds.count() * refill_rate;

    if(bucket.tokens > capacity) bucket.tokens = capacity;

    
    // Evaluate if the request can be processed
    if(bucket.tokens >= 1.0) {
        bucket.tokens -= 1.0;
        bucket.last_access = now;

        return true;
    }



    return false;      // request dropped
}