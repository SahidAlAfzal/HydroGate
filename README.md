# 🌊 HydroGate

**HydroGate** is a high-performance, custom API Gateway and Reverse Proxy written in C++17. It sits in front of your backend services to intercept HTTP requests, enforce rate limiting using a distributed Redis-backed Token Bucket algorithm, and securely proxy allowed requests.

## 🚀 Key Features

- **Blazing Fast**: Written in modern C++17 for minimal overhead and maximum performance.
- **Distributed Rate Limiting**: Uses a Redis-backed Token Bucket algorithm to track and limit requests per client IP across multiple instances of the gateway.
- **Reverse Proxy**: Intelligently proxies HTTP methods (`GET`, `POST`, `PUT`, `DELETE`, etc.), headers, and payloads to your downstream backend securely.
- **Header Sanitization**: Strips hop-by-hop headers to prevent HTTP smuggling and safely regenerates required headers.
- **Docker Ready**: Features a highly-optimized, multi-stage Docker build resulting in a tiny, secure runtime image.

---

## 🛠️ Tech Stack

- **Language**: C++17
- **HTTP Server/Client**: [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- **Redis Client**: [redis-plus-plus](https://github.com/sewenew/redis-plus-plus) & hiredis
- **Security**: OpenSSL
- **Build System**: CMake

---

## ⚙️ Environment Variables

HydroGate requires a `.env` file in the project root with the following configuration:

```env
# The connection string to your Redis instance
REDIS_URL=tcp://127.0.0.1:6379

# The downstream backend API you are proxying traffic to
MESSBOOK_URL=http://localhost:3000
```

---

## 💻 Local Development

### Prerequisites
You need the following installed on your machine:
- CMake (>= 3.10)
- A C++17 compatible compiler (Clang/GCC)
- OpenSSL (`libssl-dev`)
- Hiredis (`libhiredis-dev`)
- redis-plus-plus

### Build & Run
1. **Clone the repository:**
   ```bash
   git clone <your-repo-url>
   cd HydroGate
   ```
2. **Create the build directory and compile:**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```
3. **Run the gateway:**
   ```bash
   # Ensure you have a .env file in the root directory first!
   cd ..
   ./build/hydrogate
   ```

---

## 🐳 Docker Deployment

HydroGate uses a multi-stage Docker build to keep the final image incredibly small and secure.

1. **Build the Docker Image:**
   ```bash
   docker build -t hydrogate .
   ```

2. **Run the Container:**
   ```bash
   # Assuming you have a Redis instance accessible
   docker run -p 8080:8080 --env-file .env hydrogate
   ```

---

## 🧠 Architecture Overview

1. **Request Interception**: Incoming HTTP requests hit `proxy_handler`. The client's IP is extracted.
2. **Token Bucket Check**: The IP is checked against the Redis database. If the IP has exhausted its tokens, a `429 Too Many Requests` response is immediately returned.
3. **Proxy Execution**: If allowed, the request headers are sanitized and forwarded to the target `MESSBOOK_URL`.
4. **Response Relay**: The response from the backend is captured and streamed back to the original client. If the backend is down, a `502 Bad Gateway` is served gracefully.
