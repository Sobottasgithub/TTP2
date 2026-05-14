cmake_minimum_required(VERSION 3.10)
project(ttp2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEF_TTP2 "configure library" ON)
option(DEF_CLIENT "configure client" ON)
option(DEF_SERVER "configure server" ON)

if(DEF_TTP2)
    add_subdirectory(lib/ttp2)
endif()

if(DEF_CLIENT)
    add_subdirectory(test/client)
endif()

if(DEF_SERVER)
    add_subdirectory(test/server)
endif()
