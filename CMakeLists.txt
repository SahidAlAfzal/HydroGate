cmake_minimum_required(VERSION 3.10)
project(HydroGate)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# specify where to find header files
include_directories(include)

# Create the executable from the source files
add_executable(hydrogate src/main.cpp src/TokenBucket.cpp)