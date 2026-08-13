#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <fstream> // Required for reading the .env file

#define CPPHTTPLIB_OPENSSL_SUPPORT 
#include <httplib.h> 
#include "RedisTokenBucket.h"

// Native C++ .env parser (No external dependencies needed)
void load_env(const std::string& filename = ".env") {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open .env file.\n";
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        auto delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            
            // Strip inline comments
            auto comment_pos = value.find('#');
            if (comment_pos != std::string::npos) {
                value = value.substr(0, comment_pos);
            }
            
            // Trim whitespace
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            key.erase(key.find_last_not_of(" \t\r\n") + 1);

            // Inject the key-value pair directly into the OS environment
            setenv(key.c_str(), value.c_str(), 1);
        }
    }
}

// ---------------------------------------------------------
// HTTP Proxy Handler (Separation of Concerns)
// ---------------------------------------------------------
httplib::Server::HandlerResponse proxy_handler(
    const httplib::Request& req, 
    httplib::Response& res, 
    RedisTokenBucket& rate_limiter, 
    httplib::Client& messbook_backend
) {
    std::string client_ip = req.remote_addr;
    
    std::cout << "Intercepted: " << req.method << " " << req.target << " | IP: " << client_ip;

    // A. Check Redis Token Bucket
    try {
        if (!rate_limiter.allow_request(client_ip)) {
            res.status = 429;
            res.set_content("{\"error\": \"Rate limit exceeded. Please wait.\"}", "application/json");
            std::cout << " -> [HTTP 429: BLOCKED]\n";
            return httplib::Server::HandlerResponse::Handled; 
        }
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content("{\"error\": \"Internal Rate Limiter Error\", \"details\": \"" + std::string(e.what()) + "\"}", "application/json");
        std::cout << " -> [HTTP 500: REDIS ERROR: " << e.what() << "]\n";
        return httplib::Server::HandlerResponse::Handled;
    }

    std::cout << " -> [PROXYING TO MESSBOOK]\n";
    
    // B. Reconstruct headers (Crucial for Postman JWTs & Preventing HTTP Smuggling)
    httplib::Headers forward_headers;
    for (const auto& header : req.headers) {
        // STRIP HOP-BY-HOP HEADERS AND PAYLOAD HEADERS
        // cpp-httplib will safely regenerate Content-Length and Content-Type for us.
        if (header.first != "Host" && 
            header.first != "Content-Length" && 
            header.first != "Content-Type") {
            forward_headers.emplace(header.first, header.second);
        }
    }

    httplib::Result backend_res;
    std::string content_type = req.get_header_value("Content-Type");

    // C. Universal Method Forwarding
    if (req.method == "GET") {
        backend_res = messbook_backend.Get(req.target, forward_headers);
    } 
    else if (req.method == "POST") {
        backend_res = messbook_backend.Post(req.target, forward_headers, req.body, content_type.c_str());
    } 
    else if (req.method == "PUT") {
        backend_res = messbook_backend.Put(req.target, forward_headers, req.body, content_type.c_str());
    } 
    else if (req.method == "PATCH") {
        backend_res = messbook_backend.Patch(req.target, forward_headers, req.body, content_type.c_str());
    } 
    else if (req.method == "DELETE") {
        backend_res = messbook_backend.Delete(req.target, forward_headers);
    } 
    else if (req.method == "OPTIONS") {
        backend_res = messbook_backend.Options(req.target, forward_headers);
    } 
    else if (req.method == "HEAD") {
        backend_res = messbook_backend.Head(req.target, forward_headers);
    }
    else {
        res.status = 405; 
        return httplib::Server::HandlerResponse::Handled;
    }

    // D. Return the exact FastAPI response
    if (backend_res) {
        res.status = backend_res->status;
        for (const auto& header : backend_res->headers) {
            if (header.first != "Content-Length" && header.first != "Transfer-Encoding") {
                res.set_header(header.first, header.second);
            }
        }
        res.set_content(backend_res->body, backend_res->get_header_value("Content-Type").c_str());
    } else {
        res.status = 502; 
        res.set_content("{\"error\": \"Production MessBook API is currently unreachable.\"}", "application/json");
    }

    return httplib::Server::HandlerResponse::Handled;
}

// ---------------------------------------------------------
// Helper to clean environment variables from Render dashboard
// ---------------------------------------------------------
std::string clean_env_var(const char* env_val) {
    if (!env_val) return "";
    std::string str = env_val;
    auto start = str.find_first_not_of(" \t\r\n\"'");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(start, end - start + 1);
}

// ---------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------
int main() {
    // 1. Load the environment variables natively
    load_env();
    
    std::string redis_url = clean_env_var(std::getenv("REDIS_URL"));
    std::string messbook_url = clean_env_var(std::getenv("MESSBOOK_URL")); 

    if (redis_url.empty() || messbook_url.empty()) {
        throw std::runtime_error("[CRITICAL] REDIS_URL or MESSBOOK_URL is not set in .env");
    }
    
    std::cout << "--- Security: Environment Variables Loaded ---\n";
    std::cout << "[DEBUG] Redis URL Scheme starts with: " << redis_url.substr(0, 10) << "...\n";

    // 2. Initialize HydroGate rate limiter (Dependency)
    RedisTokenBucket rate_limiter(redis_url.c_str(), 5.0, 1.0);
    httplib::Server gateway;

    // 3. Initialize the proxy client to your live FastAPI production server (Dependency)
    httplib::Client messbook_backend(messbook_url.c_str());
    messbook_backend.enable_server_certificate_verification(false); 

    // 4. The Universal Proxy Router (Dependency Injection)
    gateway.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) {
        return proxy_handler(req, res, rate_limiter, messbook_backend);
    });

    std::cout << "--- HydroGate Universal Proxy Live ---\n";
    std::cout << "Listening locally on Port 8080 and proxying to " << messbook_url << "\n";

    gateway.listen("0.0.0.0", 8080);
    return 0;
}