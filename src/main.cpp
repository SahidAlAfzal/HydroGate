#include <iostream>
#include <thread>
#include "TokenBucket.h"

int main() {
    // Initialize with a capacity of 5 tokens, refilling at 1 token per second
    TokenBucket rate_limiter(5.0, 1.0);
    std::string test_ip = "192.168.1.50";

    std::cout << "--- HydroGate (Phase 1) Initialized ---\n";
    std::cout << "Sending 7 rapid requests to test capacity limit...\n";

    // Simulate a burst of traffic
    for (int i = 1; i <= 7; ++i) {
        bool allowed = rate_limiter.allow_request(test_ip);
        std::cout << "Request " << i << ": " 
                  << (allowed ? "[ACCEPTED]" : "[DROPPED - 429]") << "\n";
    }

    std::cout << "\nWaiting 2.5 seconds for tokens to refill...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    std::cout << "\nSending 1 request after refill...\n";
    bool allowed = rate_limiter.allow_request(test_ip);
    std::cout << "Request 8: " << (allowed ? "[ACCEPTED]" : "[DROPPED - 429]") << "\n";

    return 0;
}