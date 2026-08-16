add_library(window SHARED)

target_sources(window PRIVATE
    src/window.cpp
)

target_link_libraries(window PUBLIC 
    glfw
    glm::glm
)

target_include_directories(window PUBLIC include)

