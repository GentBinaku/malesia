#pragma once

#include <concepts>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <malesia/window.hpp>

template <typename T> constexpr bool is_expected_v = false;

template <typename T, typename E>
constexpr bool is_expected_v<std::expected<T, E>> = true;

template <typename T>
concept is_expected = is_expected_v<std::remove_cvref_t<T>>;

// Monadic chaining for std::expected: `parse(x) | validate | build`.
template <typename T, typename E, typename Function>
  requires std::invocable<Function, T> &&
           is_expected<std::invoke_result_t<Function, T>>
constexpr auto operator|(std::expected<T, E> &&ex, Function &&f)
    -> std::invoke_result_t<Function, T> {
  if (!ex) {
    return std::unexpected(std::move(ex).error());
  }
  return std::invoke(std::forward<Function>(f), *std::move(ex));
}

namespace malesia::render {

// Opaque GPU resource handles. The underlying value is a 1-based index into a
// Renderer-owned table; 0 means "not a resource".
enum class MeshHandle : std::uint32_t { Invalid = 0 };
enum class ProgramHandle : std::uint32_t { Invalid = 0 };

// The whole set of submission paths a frame is bucketed into. Content scales to
// millions of objects; this list does not grow with it.
enum class RenderPath : std::uint8_t {
  Opaque = 0,
  Transparent,
  Wireframe,
};

// One draw call's worth of data. Trivially copyable, cheap to sort by the
// thousand. A tree, a crate, an NPC all reduce to one of these.
struct RenderItem {
  MeshHandle mesh = MeshHandle::Invalid;
  ProgramHandle program = ProgramHandle::Invalid;
  glm::mat4 model = glm::mat4(1.0f);
  glm::vec3 color = glm::vec3(1.0f);
  RenderPath path = RenderPath::Opaque;
  float depth = 0.0f; // view-space distance, used to order transparency
};

// Compile-time contract for a game-side object that can contribute one draw.
// Game objects do not render themselves - they hand back a RenderItem and the
// Renderer decides when and how it actually hits the GPU.
template <typename T>
concept Renderable = requires(const T &obj) {
  { obj.extract() } -> std::same_as<RenderItem>;
};

// Pure data: the list of draws for one frame. No GPU state lives here, so it is
// cheap to clear and rebuild every frame ("extract").
class Scene {
public:
  auto clear() -> void { _items.clear(); }

  auto submit(const RenderItem &item) -> void { _items.push_back(item); }

  template <Renderable T> auto submit(const T &obj) -> void {
    _items.push_back(obj.extract());
  }

  auto items() const -> std::span<const RenderItem> { return _items; }

private:
  std::vector<RenderItem> _items;
};

// Owns GPU resources and turns a Scene into GPU work.
//
// Backend: Vulkan 1.3 (dynamic rendering + synchronization2), brought up with
// vk-bootstrap; device memory via VMA; GLSL compiled to SPIR-V at load time
// with shaderc. All of that lives behind Impl so this header stays free of
// Vulkan headers.
class Renderer {
public:
  explicit Renderer(window::Window &window);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  Renderer(Renderer &&) noexcept;
  Renderer &operator=(Renderer &&) noexcept;

  // How a mesh's vertex buffer will be used after upload. The set is closed and
  // finite, so it is a variant of empty tag types rather than an open enum:
  // call sites read as `Renderer::Dynamic{}`, and an alternative can later grow
  // a payload (e.g. a reserved size) without touching the others.
  struct Static {};  // uploaded once, drawn for many frames (default)
  struct Dynamic {}; // re-uploaded frequently
  struct Stream {};  // uploaded, used for a few draws, discarded
  using Usage = std::variant<Static, Dynamic, Stream>;

  // The primitive the vertex stream assembles into. Also a closed set, kept as
  // tag types for the same reasons as Usage.
  struct Triangles {};
  struct TriangleStrip {};
  struct TriangleFan {};
  struct Lines {};
  struct LineStrip {};
  struct LineLoop {};
  struct Points {};
  using Primitive = std::variant<Triangles, TriangleStrip, TriangleFan, Lines,
                                 LineStrip, LineLoop, Points>;

  // Upload a flat array of vec3 positions. Returns a handle usable in
  // RenderItem::mesh.
  auto uploadMesh(std::span<const float> positions,
                  Primitive primitive = Triangles{}, Usage usage = Static{})
      -> MeshHandle;

  auto createProgram(std::string const &vertexSource,
                     std::string const &fragmentSource)
      -> std::expected<ProgramHandle, std::string>;

  // Sort the scene's items into path / program / depth order so state changes
  // batch, then record and submit one frame.
  auto render(const Scene &scene, glm::mat4 const &viewProj) -> void;

  // Immediate-mode debug overlay. The callback runs each frame between ImGui's
  // NewFrame and Render, so it may call any ImGui:: function directly. Pass a
  // default-constructed (empty) function to draw nothing. UI code lives in the
  // app; the renderer only owns ImGui's lifetime and per-frame plumbing.
  using DebugUiFn = std::function<void()>;
  auto setDebugUi(DebugUiFn fn) -> void;

private:
  struct Impl;
  std::indirect<Impl> _impl;
};

auto loadShaderSource(std::filesystem::path const &path)
    -> std::expected<std::string, std::string>;

} // namespace malesia::render
