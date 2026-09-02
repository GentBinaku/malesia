#pragma once

// clang-format off
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
// clang-format on

namespace malesia::window {
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
  auto handle() const -> GLFWwindow *;
  auto setKeyCallback(KeyCallback callback) -> void;

  // --- Vulkan integration -------------------------------------------------

  // Instance extensions GLFW needs to create a surface on this platform.
  static auto requiredInstanceExtensions() -> std::vector<const char *>;

  // Create a VkSurfaceKHR for this window. Throws on failure. The caller owns
  // the surface and must destroy it before the instance.
  auto createSurface(VkInstance instance) const -> VkSurfaceKHR;

  // Current framebuffer size in pixels (may differ from the requested size on
  // HiDPI displays, and is {0, 0} while minimized).
  auto framebufferSize() const -> std::pair<uint32_t, uint32_t>;

  // Set by the framebuffer-size callback; the renderer polls and clears it to
  // drive swapchain recreation.
  auto framebufferResized() const -> bool;
  auto clearFramebufferResized() -> void;

  // Block (pumping events) while the window is minimized to a zero-size
  // framebuffer, so the renderer never builds a 0x0 swapchain.
  auto waitWhileMinimized() const -> void;

private:
  using WindowHandle =
      std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>;

  static auto handleKeyEvent(GLFWwindow *window, int key, int scancode,
                             int action, int mods) -> void;
  static auto handleFramebufferResize(GLFWwindow *window, int width, int height)
      -> void;

  WindowHandle _window;
  uint32_t _width;
  uint32_t _height;
  bool _framebufferResized = false;
  KeyCallback _keyCallback;
};
} // namespace malesia::window
