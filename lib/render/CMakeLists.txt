add_library(render SHARED)

target_sources(render
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/render.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/vma.cpp"
    PUBLIC
        FILE_SET headers TYPE HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/include/malesia/render.hpp"
)

# VMA is a large third-party header; don't hold its implementation TU to our
# warning level.
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/vma.cpp"
    PROPERTIES COMPILE_OPTIONS "-w"
)

target_link_libraries(render PUBLIC
    window
    glm::glm
    Vulkan::Vulkan
)

target_link_libraries(render PRIVATE
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
    unofficial::shaderc::shaderc
    imgui::imgui
)
