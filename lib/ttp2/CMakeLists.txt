cmake_minimum_required(VERSION 3.10)
project(ttp2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(libtasn1 REQUIRED)

include_directories(
  include
  ${LIBTASN1_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
# file(GLOB_RECURSE HEADERS "include/*.h")

add_library(ttp2 SHARED ${SOURCES} ${LIBTASN1_LIBRARIES})

target_include_directories(ttp2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR} # for libtasn1
)

# Link against the libtasn1 library
target_link_libraries(ttp2 PRIVATE tasn1)

install(TARGETS ttp2 DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
