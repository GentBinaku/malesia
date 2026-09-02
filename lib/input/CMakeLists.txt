add_library(input SHARED)

target_sources(input
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/input.cpp"
    PUBLIC
        FILE_SET headers TYPE HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/include/malesia/input.hpp"
)

target_link_libraries(input PUBLIC
    glfw
)
