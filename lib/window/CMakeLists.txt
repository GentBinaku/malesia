add_library(window SHARED)

target_sources(window
    PUBLIC
    FILE_SET modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/window.cpp"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/window_impl.cpp"
)

target_link_libraries(window PUBLIC
    glfw
    glm::glm
    glad::glad
    OpenGL::GL
)

