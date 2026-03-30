cmake_minimum_required(VERSION 3.10)
project(ttp2-client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "client/*.cpp" "client/*.h")

add_executable(ttp2-client ${SOURCES})

install(TARGETS ttp2-client DESTINATION bin)
