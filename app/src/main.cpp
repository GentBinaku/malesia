#include <print>
#include <stop_token>
#include "window/window.hpp"

auto mainLoop() -> void
{
	std::stop_token stoken = std::stop_token();

	while (!stoken.stop_requested())
	{
		glfwPollEvents();
	}
}

auto main() -> int {
	mainLoop();
	return 0;
}