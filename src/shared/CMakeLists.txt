add_library(shared STATIC
    Utils.cpp
    Utils.h
    consts.h
)

target_include_directories(shared
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
