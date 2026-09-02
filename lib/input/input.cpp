#include <malesia/input.hpp>

#include <GLFW/glfw3.h>

namespace malesia::input {
auto Input::handleKeyEvent(int key, int scancode, int action, int mods)
    -> void {
  if (action == GLFW_PRESS) {
    _keyStates[key] = true;
  } else if (action == GLFW_RELEASE) {
    _keyStates[key] = false;
  }
}

auto Input::isKeyPressed(int key) const -> bool {
  auto it = _keyStates.find(key);
  return it != _keyStates.end() && it->second;
}
} // namespace malesia::input
