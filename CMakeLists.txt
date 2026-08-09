cmake_minimum_required(VERSION 3.10)
project(HydroGate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Point to the header files
include_directories(include)
include_directories(/opt/homebrew/include)               # hiredis
include_directories(/usr/local/include)                  # redis++
include_directories(/opt/homebrew/opt/openssl@3/include) # OpenSSL

# 2. Point to the compiled library files
link_directories(/opt/homebrew/lib)                      # libhiredis
link_directories(/usr/local/lib)                         # libredis++
link_directories(/opt/homebrew/opt/openssl@3/lib)        # OpenSSL libs

# 3. Build executable
add_executable(hydrogate src/main.cpp src/RedisTokenBucket.cpp)

# 4. Explicitly link libraries AND macOS security frameworks
target_link_libraries(hydrogate hiredis redis++ ssl crypto "-framework CoreFoundation" "-framework Security")