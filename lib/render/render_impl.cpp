module;

#include <expected>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

module malesia.render;

namespace malesia {
namespace render {

namespace {

auto compileShader(GLenum type, std::string const &source)
    -> std::expected<GLuint, std::string> {
  GLuint shader = glCreateShader(type);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint success = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<size_t>(logLength), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    glDeleteShader(shader);
    return std::unexpected(std::move(log));
  }
  return shader;
}

auto linkProgram(GLuint vertexShader, GLuint fragmentShader)
    -> std::expected<GLuint, std::string> {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLint success = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<size_t>(logLength), '\0');
    glGetProgramInfoLog(program, logLength, nullptr, log.data());
    glDeleteProgram(program);
    return std::unexpected(std::move(log));
  }
  return program;
}

auto createBuffers(std::span<float> vertices) -> std::pair<GLuint, GLuint> {
  GLuint vao = 0;
  GLuint vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size_bytes()),
               vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        static_cast<void *>(0));
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return {vao, vbo};
}
} // namespace

auto loadShaderSource(std::filesystem::path const &path)
    -> std::expected<std::string, std::string> {
  std::ifstream file(path);
  if (!file) {
    return std::unexpected("failed to open shader file: " + path.string());
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  return std::move(contents).str();
}

} // namespace render
} // namespace malesia
