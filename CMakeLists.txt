cmake_minimum_required(VERSION 3.10)
project(Whisper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SERVER "Build server" OFF)
option(BUILD_CLIENT "Build client" OFF)

if(NOT BUILD_SERVER AND NOT BUILD_CLIENT)
    message(WARNING "No target selected for build.")
endif()

if(BUILD_SERVER)
    add_executable(server
        src/server.cpp
        src/server.h
        src/IPv4Addr.cpp
        src/IPv4Addr.h
    )
    target_include_directories(server PRIVATE .)
endif()

if(BUILD_CLIENT)
    add_executable(client
        src/client.cpp
        src/client.h
    )
    target_include_directories(client PRIVATE .)
endif()
