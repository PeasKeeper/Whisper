project(WhisperServer LANGUAGES CXX)

add_executable(server
    main.cpp
    Server.cpp
    Server.h
    IPv4Addr.cpp
    IPv4Addr.h
    ClientData.h
)

target_link_libraries(server
    PRIVATE
    shared
)

target_include_directories(server
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
