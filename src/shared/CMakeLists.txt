add_library(shared STATIC
    StringUtils.cpp
    StringUtils.h
    consts.h
    StopReason.h
    SocketAdapter.cpp
    SocketAdapter.h
)

target_include_directories(shared
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
