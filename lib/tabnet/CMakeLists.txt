cmake_minimum_required(VERSION 3.10)
project(tabnet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")
# file(GLOB_RECURSE HEADERS "include/*.h")

add_library(tabnet SHARED ${SOURCES})

target_include_directories(tabnet PUBLIC include)

install(TARGETS tabnet DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
