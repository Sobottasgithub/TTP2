cmake_minimum_required(VERSION 3.10)
project(ttp2-client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "./*.cpp" "./*.h")

add_executable(ttp2-client ${SOURCES})

target_link_libraries(ttp2-client PRIVATE ttp2)
find_package(Arrow REQUIRED)
target_link_libraries(ttp2-client PRIVATE Arrow::arrow_shared)

install(TARGETS ttp2-client DESTINATION bin)
