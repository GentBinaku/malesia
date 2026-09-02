// clang-format off
#include <cmath>
#include <cstdio>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <malesia/window.hpp>
#include <malesia/render.hpp>
#include <imgui.h>
// clang-format on

namespace {

constexpr std::string_view kVertexShader = R"(#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMvp;
void main() {
  gl_Position = uMvp * vec4(aPos, 1.0);
  gl_PointSize = 2.0;
}
)";

constexpr std::string_view kFragmentShader = R"(#version 460 core
out vec4 FragColor;
uniform vec3 uColor;
void main() { FragColor = vec4(uColor, 1.0); }
)";

// Flat xyz point cloud on the unit sphere.
auto makeSphere(int stacks, int slices) -> std::vector<float> {
  std::vector<float> v;
  v.insert(v.end(), {0.0f, 1.0f, 0.0f});
  for (int i = 1; i < stacks; ++i) {
    float phi = std::numbers::pi_v<float> * float(i) / float(stacks);
    for (int j = 0; j < slices; ++j) {
      float theta = 2.0f * std::numbers::pi_v<float> * float(j) / float(slices);
      v.insert(v.end(), {std::sin(phi) * std::cos(theta), std::cos(phi),
                         std::sin(phi) * std::sin(theta)});
    }
  }
  v.insert(v.end(), {0.0f, -1.0f, 0.0f});
  return v;
}

// A game-side object. It does not draw itself - it satisfies Renderable by
// handing back one RenderItem each frame.
struct Spinner {
  malesia::render::MeshHandle mesh;
  malesia::render::ProgramHandle program;
  glm::vec3 color;
  glm::vec3 axis;
  float angle;

  auto extract() const -> malesia::render::RenderItem {
    return {
        .mesh = mesh,
        .program = program,
        .model = glm::rotate(glm::mat4(1.0f), angle, axis),
        .color = color,
        .path = malesia::render::RenderPath::Opaque,
    };
  }
};

} // namespace

auto main() -> int {
  malesia::window::Window window(1920, 1080);
  window.setKeyCallback([&window](int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window.handle(), GLFW_TRUE);
    }
  });

  malesia::render::Renderer renderer(window);

  auto sphereVerts = makeSphere(100, 100);
  auto sphereMesh =
      renderer.uploadMesh(sphereVerts, malesia::render::Renderer::Points{});
  auto program = renderer.createProgram(std::string(kVertexShader),
                                        std::string(kFragmentShader));
  if (!program) {
    std::fputs(program.error().c_str(), stderr);
    return 1;
  }

  Spinner sphere{
      sphereMesh, *program, {0.4f, 0.8f, 1.0f}, {0.0f, 1.0f, 0.0f}, 0.0f};

  glm::mat4 proj =
      glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 viewProj = proj * view;

  malesia::render::Scene scene;

  bool showOverlay = false;
  renderer.setDebugUi([&] {
    if (!showOverlay)
      return;
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("malesia", &showOverlay,
                     ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav)) {
      ImGui::Text("%.1f FPS  (%.2f ms)", ImGui::GetIO().Framerate,
                  1000.0f / ImGui::GetIO().Framerate);
      ImGui::Text("sphere angle: %.2f rad", sphere.angle);
      ImGui::TextDisabled("scaffold: clear + overlay, no geometry yet");
    }
    ImGui::End();
  });

  while (!window.shouldClose()) {
    window.pollEvents();

    sphere.angle = float(glfwGetTime());

    scene.clear();
    scene.submit(sphere); // Renderable overload -> sphere.extract()
    renderer.render(scene, viewProj);
  }

  return 0;
}
