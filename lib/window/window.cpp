// clang-format off
#include <malesia/window.hpp>
#include <stdexcept>
// clang-format on

namespace malesia::window {
Window::Window(uint32_t width, uint32_t height)
    : _window(nullptr, &glfwDestroyWindow), _width(width), _height(height) {
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("failed to initialize GLFW");
  }

  if (glfwVulkanSupported() != GLFW_TRUE) {
    glfwTerminate();
    throw std::runtime_error("Vulkan loader not found by GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  _window.reset(glfwCreateWindow(static_cast<int>(_width),
                                 static_cast<int>(_height), "Malesia (Vulkan)",
                                 nullptr, nullptr));
  if (!_window) {
    glfwTerminate();
    throw std::runtime_error("failed to create GLFW window");
  }

  glfwSetWindowUserPointer(_window.get(), this);
  glfwSetKeyCallback(_window.get(), &Window::handleKeyEvent);
  glfwSetFramebufferSizeCallback(_window.get(), &Window::handleFramebufferResize);
}

Window::~Window() {
  _window.reset();
  glfwTerminate();
}

auto Window::shouldClose() const -> bool {
  return glfwWindowShouldClose(_window.get()) == GLFW_TRUE;
}

auto Window::pollEvents() const -> void { glfwPollEvents(); }

auto Window::handle() const -> GLFWwindow * { return _window.get(); }

auto Window::setKeyCallback(KeyCallback callback) -> void {
  _keyCallback = std::move(callback);
}

auto Window::requiredInstanceExtensions() -> std::vector<const char *> {
  uint32_t count = 0;
  const char **names = glfwGetRequiredInstanceExtensions(&count);
  if (names == nullptr) {
    throw std::runtime_error(
        "glfwGetRequiredInstanceExtensions returned null (no Vulkan support)");
  }
  return {names, names + count};
}

auto Window::createSurface(VkInstance instance) const -> VkSurfaceKHR {
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (glfwCreateWindowSurface(instance, _window.get(), nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface");
  }
  return surface;
}

auto Window::framebufferSize() const -> std::pair<uint32_t, uint32_t> {
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(_window.get(), &width, &height);
  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

auto Window::framebufferResized() const -> bool { return _framebufferResized; }

auto Window::clearFramebufferResized() -> void { _framebufferResized = false; }

auto Window::waitWhileMinimized() const -> void {
  auto [width, height] = framebufferSize();
  while (width == 0 || height == 0) {
    glfwWaitEvents();
    std::tie(width, height) = framebufferSize();
  }
}

auto Window::handleKeyEvent(GLFWwindow *window, int key, int scancode,
                            int action, int mods) -> void {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self && self->_keyCallback) {
    self->_keyCallback(key, scancode, action, mods);
  }
}

auto Window::handleFramebufferResize(GLFWwindow *window, int /*width*/,
                                     int /*height*/) -> void {
  auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (self) {
    self->_framebufferResized = true;
  }
}
} // namespace malesia::window
