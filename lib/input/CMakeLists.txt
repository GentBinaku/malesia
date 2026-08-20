add_library(input SHARED)
target_sources(input
    PUBLIC
    FILE_SET modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/input_module.cppm"
)

target_link_libraries(input PUBLIC
    glfw
)

