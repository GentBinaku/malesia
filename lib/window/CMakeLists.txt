add_library(window SHARED)

target_sources(window
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/window.cpp"
    PUBLIC
        FILE_SET headers TYPE HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/include/malesia/window.hpp"
)

target_link_libraries(window PUBLIC
    glfw
    glm::glm
    Vulkan::Vulkan
)
