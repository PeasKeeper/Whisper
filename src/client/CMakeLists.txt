project(WhisperClient LANGUAGES CXX)

add_executable(client
    main.cpp
    Client.cpp
    Client.h
    UiManager.cpp
    UiManager.h
)

target_link_libraries(client
    PRIVATE
    shared
    ftxui::screen
    ftxui::dom
    ftxui::component
)

target_include_directories(client
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
