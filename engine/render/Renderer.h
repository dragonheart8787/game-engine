#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <GLES3/gl3.h>

#include "engine/math/Math.h"

namespace engine::render {

struct Renderable {
  engine::math::Vec3 position{0.0f};
  engine::math::Vec3 scale{1.0f};
  engine::math::Vec4 color{1.0f};
};

struct DebugDrawCmd {
  std::vector<engine::math::Vec3> points;
  engine::math::Vec4 color{1.0f};
};

class Renderer {
public:
  bool initialize();
  void beginFrame(const engine::math::Vec4& clearColor);
  void submit(const Renderable& renderable, const engine::math::Mat4& viewProj);
  void submitDebug(const DebugDrawCmd& cmd, const engine::math::Mat4& viewProj);
  void endFrame();
  void shutdown();
  void hotReload();

private:
  bool loadShader();
  GLuint program_ = 0;
  GLuint vao_ = 0;
  GLuint vbo_ = 0;
  GLuint ebo_ = 0;
  std::string vertexPath_;
  std::string fragmentPath_;
  std::filesystem::file_time_type vertexTimestamp_{};
  std::filesystem::file_time_type fragmentTimestamp_{};
};

}  // namespace engine::render
