add_library(render SHARED)
target_sources(render
    PUBLIC
    FILE_SET modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/render.cpp"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/render_impl.cpp"

)

target_link_libraries(render PUBLIC
    glfw
    glad::glad
    OpenGL::GL
)

