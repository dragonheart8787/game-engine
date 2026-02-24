#include "engine/render/RenderBackend.h"

namespace engine::render {

bool VulkanBackend::beginRenderPass(const RenderPassDescription& pass, std::string& error) {
  GraphicsPipelineDescription placeholderPipeline{};
  mapping_ = RenderApiContract::buildMentalModel(pass, placeholderPipeline);
  return RenderApiContract::validateRenderPass(pass, error);
}

bool VulkanBackend::bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) {
  return RenderApiContract::validatePipeline(pipeline, error);
}

bool VulkanBackend::transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) {
  return RenderApiContract::validateTransitions(transitions, error);
}

void VulkanBackend::endRenderPass() {
  mapping_ = {};
}

bool D3D12Backend::beginRenderPass(const RenderPassDescription& pass, std::string& error) {
  if (!RenderApiContract::validateRenderPass(pass, error)) {
    return false;
  }
  commandTrace_ = "OMSetRenderTargets/BeginRenderPass-ready";
  return true;
}

bool D3D12Backend::bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) {
  return RenderApiContract::validatePipeline(pipeline, error);
}

bool D3D12Backend::transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) {
  if (!RenderApiContract::validateTransitions(transitions, error)) {
    return false;
  }
  commandTrace_ += ";ResourceBarrier(Transition)";
  return true;
}

void D3D12Backend::endRenderPass() {
  commandTrace_.clear();
}

bool MetalBackend::beginRenderPass(const RenderPassDescription& pass, std::string& error) {
  if (!RenderApiContract::validateRenderPass(pass, error)) {
    return false;
  }
  encoderTrace_ = "MTLRenderPassDescriptor";
  return true;
}

bool MetalBackend::bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) {
  if (!RenderApiContract::validatePipeline(pipeline, error)) {
    return false;
  }
  encoderTrace_ += ";MTLRenderPipelineState+MTLDepthStencilState";
  return true;
}

bool MetalBackend::transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) {
  return RenderApiContract::validateTransitions(transitions, error);
}

void MetalBackend::endRenderPass() {
  encoderTrace_.clear();
}


bool OpenGlesBackend::beginRenderPass(const RenderPassDescription& pass, std::string& error) {
  return RenderApiContract::validateRenderPass(pass, error);
}

bool OpenGlesBackend::bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) {
  return RenderApiContract::validatePipeline(pipeline, error);
}

bool OpenGlesBackend::transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) {
  return RenderApiContract::validateTransitions(transitions, error);
}

void OpenGlesBackend::endRenderPass() {}

std::unique_ptr<IRenderBackend> createBackend(GraphicsBackend backend) {
  switch (backend) {
    case GraphicsBackend::Vulkan:
      return std::make_unique<VulkanBackend>();
    case GraphicsBackend::D3D12:
      return std::make_unique<D3D12Backend>();
    case GraphicsBackend::Metal:
      return std::make_unique<MetalBackend>();
    case GraphicsBackend::OpenGLES:
      return std::make_unique<OpenGlesBackend>();
  }
  return nullptr;
}

}  // namespace engine::render
