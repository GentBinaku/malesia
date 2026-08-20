//
// Created by gbinaku on 8/16/2026.
//

#ifndef MALESIA_WINDOWS_HPP
#define MALESIA_WINDOWS_HPP
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>

namespace malesia
{
namespace window
{
    class Window
    {
    public:
        Window(uint32_t width, uint32_t height);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        auto shouldClose() const -> bool;
        auto pollEvents() const -> void;
        auto swapBuffers() const -> void;
        auto handle() const -> GLFWwindow*;
    private:
        using WindowHandle = std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>;

        WindowHandle _window;
        uint32_t _width;
        uint32_t _height;
    };
}
}
#endif //MALESIA_WINDOWS_HPP
