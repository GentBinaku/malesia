#pragma once

#include <GLFW/glfw3.h>
#include <unordered_map>

namespace malesia::input {
class Input {
public:
  auto handleKeyEvent(int key, int scancode, int action, int mods) -> void;
  auto isKeyPressed(int key) const -> bool;

private:
  std::unordered_map<int, bool> _keyStates;
};
} // namespace malesia::input
