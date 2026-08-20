module;

#include <GLFW/glfw3.h>
#include <unordered_map>

export module malesia.input;

export namespace malesia
{
namespace input
{
    class Input
    {
    public:
        auto handleKeyEvent(int key, int scancode, int action, int mods) -> void;
        auto isKeyPressed(int key) const -> bool;

    private:
        std::unordered_map<int, bool> _keyStates;
    };
}
}

namespace malesia
{
namespace input
{
    auto Input::handleKeyEvent(int key, int scancode, int action, int mods) -> void
    {
        if (action == GLFW_PRESS)
        {
            _keyStates[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            _keyStates[key] = false;
        }
    }

    auto Input::isKeyPressed(int key) const -> bool
    {
        auto it = _keyStates.find(key);
        return it != _keyStates.end() && it->second;
    }
}
}
