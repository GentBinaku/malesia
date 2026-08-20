//
// Created by gbinaku on 8/16/2026.
//
#include <stdexcept>
#include <window/window.hpp>

namespace malesia {
namespace window {
Window::Window(uint32_t width, uint32_t height)
    : _window(nullptr, &glfwDestroyWindow), _width(width), _height(height) {
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  _window.reset(glfwCreateWindow(static_cast<int>(_width),
                                 static_cast<int>(_height), "OpenGL", nullptr,
                                 nullptr));
  if (!_window) {
    glfwTerminate();
    throw std::runtime_error("failed to create GLFW window");
  }

  glfwMakeContextCurrent(_window.get());
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&glfwGetProcAddress))) {
    glfwTerminate();
    throw std::runtime_error("failed to load OpenGL functions");
  }
  glfwSwapInterval(1);

  glfwSetWindowUserPointer(_window.get(), this);
  glfwSetKeyCallback(_window.get(), &Window::handleKeyEvent);
}

Window::~Window() {
  _window.reset();
  glfwTerminate();
}

auto Window::shouldClose() const -> bool {
  return glfwWindowShouldClose(_window.get()) == GLFW_TRUE;
}

auto Window::pollEvents() const -> void { glfwPollEvents(); }

auto Window::swapBuffers() const -> void { glfwSwapBuffers(_window.get()); }

auto Window::handle() const -> GLFWwindow * { return _window.get(); }

auto Window::setKeyCallback(KeyCallback callback) -> void {
  _keyCallback = std::move(callback);
}

auto Window::handleKeyEvent(GLFWwindow *window, int key, int scancode,
                            int action, int mods) -> void {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self && !self->_keyCallback) {
    self->_keyCallback(key, scancode, action, mods);
  }
}
} // namespace window
} // namespace malesia
