//
// Created by gbinaku on 8/16/2026.
//

#ifndef MALESIA_WINDOWS_HPP
#define MALESIA_WINDOWS_HPP
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

namespace malesia 
{ 
namespace window
{
    class Window
    {
    public:
        Window();
        ~Window();
        auto renderWindow() -> void;
    private:
        std::unique_ptr<GLFWwindow> _window;
        uint32_t _width;
        uint32_t _height;
    };
} 
}
#endif //MALESIA_WINDOWS_HPP
