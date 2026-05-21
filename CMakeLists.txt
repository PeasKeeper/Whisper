cmake_minimum_required(VERSION 3.12)
project(Whisper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SERVER "Build server" ON)
option(BUILD_CLIENT "Build client" ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/shared)

if(BUILD_SERVER)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/FTXUI)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/client)
endif()
