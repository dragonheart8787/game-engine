#include "engine/platform/PlatformServices.h"
#include "engine/render/RenderBackend.h"

#include <cassert>

int main() {
  engine::platform::VirtualFileSystem vfs;
  assert(vfs.mount("/assets", "assets"));
  assert(vfs.exists("/assets/scenes/worldstate.json"));

  engine::platform::MonotonicFixedTimeSource time;
  const auto before = time.fixedTick();
  time.advanceFixedTick();
  assert(time.fixedTick() == before + 1);

  engine::platform::CrashReporter crash;
  crash.install();
  crash.capture({"test", 42, "build-local"});
  assert(crash.lastCrash().has_value());

  auto backend = engine::render::createBackend(engine::render::GraphicsBackend::Vulkan);
  assert(backend);

  engine::render::RenderPassDescription pass;
  pass.name = "main";
  pass.attachments.push_back({engine::render::TextureFormat::RGBA8_UNORM,
                              engine::render::LoadOp::Clear,
                              engine::render::StoreOp::Store,
                              engine::render::ResourceState::Undefined,
                              engine::render::ResourceState::Present});
  pass.subpasses.push_back({{0}, -1, {}});

  engine::render::GraphicsPipelineDescription pipeline;
  pipeline.name = "main";
  pipeline.shader.vsPath = "basic.vert";
  pipeline.shader.psPath = "basic.frag";
  pipeline.renderTargetCount = 1;
  pipeline.renderTargetFormats[0] = engine::render::TextureFormat::RGBA8_UNORM;

  std::string error;
  assert(backend->beginRenderPass(pass, error));
  assert(backend->bindGraphicsPipeline(pipeline, error));
  assert(backend->transitionResources({{1, engine::render::ResourceState::Undefined,
                                         engine::render::ResourceState::RenderTarget}},
                                      error));
  backend->endRenderPass();

  return 0;
}
