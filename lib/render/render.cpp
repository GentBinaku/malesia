module;

#include <expected>
#include <filesystem>
#include <functional>
#include <glad/glad.h>
#include <span>
#include <string>
#include <utility>
#include <vector>

export module malesia.render;

using namespace std;

template <typename T> constexpr bool is_expected_v = false;

template <typename T, typename E>
constexpr bool is_expected_v<expected<T, E>> = true;

template <typename T>
concept is_expected = is_expected_v<remove_cvref_t<T>>;

export template <typename T, typename E, typename Function>
  requires std::invocable<Function, T> &&
           is_expected<invoke_result_t<Function, T>>
constexpr auto operator|(std::expected<T, E> &&ex, Function &&f)
    -> invoke_result_t<Function, T> {
  if (!ex) {
    return std::unexpected(std::move(ex).error());
  }
  return std::invoke(std::forward<Function>(f), *std::move(ex));
}

export namespace malesia {
namespace render {

class RenderObject {
public:
  RenderObject();
  ~RenderObject();

  RenderObject(const RenderObject &) = delete;
  RenderObject &operator=(const RenderObject &) = delete;

  RenderObject(RenderObject &&other) noexcept;
  RenderObject &operator=(RenderObject &&other) noexcept;

  virtual auto draw() const -> void;

private:
  virtual auto release() -> void;
};

auto renderObject(std::span<float> vertices, std::string const &vertex_shader,
                  std::string const &fragment_shader)
    -> std::expected<RenderObject, std::string>;

auto loadShaderSource(std::filesystem::path const &path)
    -> std::expected<std::string, std::string>;

class Scene {
public:
  Scene() = default;

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

  Scene(Scene &&) noexcept = default;
  Scene &operator=(Scene &&) noexcept = default;

  auto addObject(RenderObject object) -> void;
  auto draw() const -> void;

private:
  std::vector<std::function<void(RenderObject)>> _drawFunctions;
};

} // namespace render
} // namespace malesia
