#include <cassert>
#include <iostream>

#include "engine/render/RenderAPI.h"

int main() {
  using namespace engine::render;

  RenderPassDescription pass;
  pass.name = "MainPass";
  pass.attachments = {
      {TextureFormat::RGBA8_UNORM, LoadOp::Clear, StoreOp::Store, ResourceState::Undefined, ResourceState::Present},
      {TextureFormat::D24S8, LoadOp::Clear, StoreOp::Store, ResourceState::Undefined, ResourceState::DepthWrite}
  };
  pass.subpasses = {
      {{0}, 1, {}}
  };

  GraphicsPipelineDescription pso;
  pso.name = "MainPSO";
  pso.shader.vsPath = "assets/shaders/basic.vert";
  pso.shader.psPath = "assets/shaders/basic.frag";
  pso.renderTargetCount = 1;
  pso.renderTargetFormats[0] = TextureFormat::RGBA8_UNORM;
  pso.depthStencilFormat = TextureFormat::D24S8;

  std::string error;
  assert(RenderApiContract::validateRenderPass(pass, error));
  assert(RenderApiContract::validatePipeline(pso, error));

  std::vector<ResourceTransition> transitions = {
      {1, ResourceState::Present, ResourceState::RenderTarget},
      {1, ResourceState::RenderTarget, ResourceState::Present}
  };
  assert(RenderApiContract::validateTransitions(transitions, error));

  const auto model = RenderApiContract::buildMentalModel(pass, pso);
  assert(!model.vkRenderPass.empty());
  assert(!model.d3d12OmSetRenderTargets.empty());
  assert(!model.metalRenderPassDescriptor.empty());

  std::cout << "render_contract_tests passed\n";
  return 0;
}
