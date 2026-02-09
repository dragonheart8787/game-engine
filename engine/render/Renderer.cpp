#include "engine/render/Renderer.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>

namespace engine::render {

namespace {
const float kQuadVertices[] = {
    -0.5f, 0.0f, -0.5f,
     0.5f, 0.0f, -0.5f,
     0.5f, 0.0f,  0.5f,
    -0.5f, 0.0f,  0.5f,
};
const unsigned int kQuadIndices[] = {0, 1, 2, 2, 3, 0};
}

static std::string readFile(const std::string& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

static GLuint compileShader(GLenum type, const std::string& source) {
  GLuint shader = glCreateShader(type);
  const char* data = source.c_str();
  glShaderSource(shader, 1, &data, nullptr);
  glCompileShader(shader);
  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

bool Renderer::initialize() {
  vertexPath_ = "assets/shaders/basic.vert";
  fragmentPath_ = "assets/shaders/basic.frag";

  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

  glBindVertexArray(0);

  return loadShader();
}

void Renderer::beginFrame(const engine::math::Vec4& clearColor) {
  glViewport(0, 0, 1280, 720);
  glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
}

void Renderer::submit(const Renderable& renderable, const engine::math::Mat4& viewProj) {
  glUseProgram(program_);

  const engine::math::Mat4 model = glm::translate(engine::math::Mat4(1.0f), renderable.position) *
      glm::scale(engine::math::Mat4(1.0f), renderable.scale);
  const engine::math::Mat4 mvp = viewProj * model;

  const GLint mvpLoc = glGetUniformLocation(program_, "uMvp");
  const GLint colorLoc = glGetUniformLocation(program_, "uColor");
  glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
  glUniform4fv(colorLoc, 1, &renderable.color[0]);

  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glBindVertexArray(0);
}

void Renderer::endFrame() {
  glUseProgram(0);
}

void Renderer::shutdown() {
  if (program_) {
    glDeleteProgram(program_);
    program_ = 0;
  }
  if (vbo_) {
    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
  }
  if (vao_) {
    glDeleteVertexArrays(1, &vao_);
    vao_ = 0;
  }
}

void Renderer::hotReload() {
  if (vertexPath_.empty() || fragmentPath_.empty()) {
    return;
  }
  const auto vertexTime = std::filesystem::last_write_time(vertexPath_);
  const auto fragmentTime = std::filesystem::last_write_time(fragmentPath_);
  if (vertexTime != vertexTimestamp_ || fragmentTime != fragmentTimestamp_) {
    loadShader();
  }
}

bool Renderer::loadShader() {
  const std::string vertexSource = readFile(vertexPath_);
  const std::string fragmentSource = readFile(fragmentPath_);

  const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
  const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
  if (!vertexShader || !fragmentShader) {
    return false;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLint linked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    glDeleteProgram(program);
    return false;
  }

  if (program_) {
    glDeleteProgram(program_);
  }
  program_ = program;
  vertexTimestamp_ = std::filesystem::last_write_time(vertexPath_);
  fragmentTimestamp_ = std::filesystem::last_write_time(fragmentPath_);
  return true;
}

}  // namespace engine::render
