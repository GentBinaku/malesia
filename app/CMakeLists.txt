add_executable(app
    src/main.cpp
)

target_link_libraries(app PRIVATE window render imgui::imgui)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

