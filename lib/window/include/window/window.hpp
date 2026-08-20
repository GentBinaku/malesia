//
// Created by gbinaku on 8/16/2026.
//

#ifndef MALESIA_WINDOWS_HPP
#define MALESIA_WINDOWS_HPP
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <memory>
// clang-format on

namespace malesia {
namespace window {
class Window {
public:
  using KeyCallback =
      std::function<void(int key, int scancode, int action, int mods)>;

  Window(uint32_t width, uint32_t height);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  auto shouldClose() const -> bool;
  auto pollEvents() const -> void;
  auto swapBuffers() const -> void;
  auto handle() const -> GLFWwindow *;
  auto setKeyCallback(KeyCallback callback) -> void;

private:
  using WindowHandle =
      std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>;

  static auto handleKeyEvent(GLFWwindow *window, int key, int scancode,
                             int action, int mods) -> void;

  WindowHandle _window;
  uint32_t _width;
  uint32_t _height;
  KeyCallback _keyCallback;
};
} // namespace window
} // namespace malesia
#endif // MALESIA_WINDOWS_HPP
