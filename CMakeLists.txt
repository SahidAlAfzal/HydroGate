cmake_minimum_required(VERSION 3.10)
project(HydroGate)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # This generates a compile_commands.json file

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Point to the local header files
include_directories(include)

# 2. Industry Standard: Dynamically searches the host operating system for OpenSSL libraries instead of using hardcoded paths.
find_package(OpenSSL REQUIRED)

# hiredis and redis++ might not have standard Find modules on all OS, 
# so we locate them safely.
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY hiredis)
find_library(HIREDIS_SSL_LIBRARY hiredis_ssl)

find_path(REDISPP_INCLUDE_DIR sw/redis++/redis++.h)
find_library(REDISPP_LIBRARY redis++)

include_directories(${OPENSSL_INCLUDE_DIR} ${HIREDIS_INCLUDE_DIR} ${REDISPP_INCLUDE_DIR})
# Tells the compiler where to look when it sees an #include <...> statement

# 3. Build executable
add_executable(hydrogate src/main.cpp src/RedisTokenBucket.cpp)

# 4. Link libraries dynamically
target_link_libraries(hydrogate 
    ${HIREDIS_LIBRARY} 
    ${HIREDIS_SSL_LIBRARY}
    ${REDISPP_LIBRARY} 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Apple-specific security frameworks (only required if compiling natively on macOS)
if(APPLE)
    target_link_libraries(hydrogate "-framework CoreFoundation" "-framework Security")
endif()