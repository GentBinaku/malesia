add_library(input SHARED)

target_sources(window PRIVATE
    src/input.cpp
)

target_link_libraries(window PUBLIC
    glfw
    glm::glm
    glad::glad
    OpenGL::GL
)

target_include_directories(input PUBLIC include)

