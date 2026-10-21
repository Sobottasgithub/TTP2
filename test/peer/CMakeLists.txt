cmake_minimum_required(VERSION 3.10)
project(ttp2-peer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "./*.cpp" "./*.h")

add_executable(ttp2-peer ${SOURCES})

target_link_libraries(ttp2-peer PRIVATE ttp2)
find_package(Arrow REQUIRED)
target_link_libraries(ttp2-peer PRIVATE Arrow::arrow_shared)
target_link_libraries(ttp2-peer PRIVATE tablog)

install(TARGETS ttp2-peer DESTINATION bin)
