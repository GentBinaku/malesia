//
// Created by gbinaku on 8/16/2026.
//
#include <window/window.hpp>

namespace malesia 
{ 
namespace window
{
    Window::Window(uint32_t width, uint32_t height): _width(width), _height(height)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }

    auto Window::renderWindow() -> void
    {
        _window = std::make_unique<GLFWwindow>(_width, _height, "Vulkan", nullptr, nullptr); 
    }
}
}
