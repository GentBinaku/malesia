#include "window/window.hpp"

auto mainLoop(malesia::window::Window& window) -> void
{
	while (!window.shouldClose())
	{
		window.pollEvents();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		window.swapBuffers();
	}
}

auto main() -> int {
	malesia::window::Window window(800, 600);

	window.setKeyCallback([&window](int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window.handle(), GLFW_TRUE);
		}
	});

	mainLoop(window);
	return 0;
}
