cmake_minimum_required(VERSION 3.10)
project(ttp2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# client
add_subdirectory(test/client)

# server
add_subdirectory(test/server)

# Test executables
add_subdirectory(protocol)
