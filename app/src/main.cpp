#include "window/window.hpp"

auto mainLoop(malesia::window::Window& window) -> void
{
	while (!window.shouldClose())
	{
		window.pollEvents();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		window.swapBuffers();
	}
}

auto main() -> int {
	malesia::window::Window window(800, 600);
	mainLoop(window);
	return 0;
}
