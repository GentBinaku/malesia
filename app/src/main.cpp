// clang-format off
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
// clang-format on

import malesia.window;
import malesia.render;

auto mainLoop(malesia::window::Window &window, malesia::render::Scene &scene)
    -> void {
  while (!window.shouldClose()) {
    window.pollEvents();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    scene.draw();

    window.swapBuffers();
  }
}

auto main() -> int {
  malesia::window::Window window(800, 600);

  window.setKeyCallback([&window](int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window.handle(), GLFW_TRUE);
    }
  });

  // Create sphere mesh
  auto n_stacks = 100;
  auto n_slices = 100;
  std::vector<std::vector<float>> sphereMesh;
  sphereMesh.push_back({0, 1, 0});
  for (int i = 0; i < n_stacks - 1; i++) {
    auto phi = M_PI * double(i + 1) / double(n_stacks);
    for (int j = 0; j < n_slices; j++) {
      auto theta = 2.0 * M_PI * double(j) / double(n_slices);
      float x = std::sin(phi) * std::cos(theta);
      float y = std::cos(phi);
      float z = std::sin(phi) * std::sin(theta);
      sphereMesh.push_back({x, y, z});
    }
  }
  sphereMesh.push_back({0, -1, 0});

  malesia::render::Scene scene;

  mainLoop(window, scene);
  return 0;
}
