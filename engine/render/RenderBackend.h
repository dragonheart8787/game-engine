#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/render/RenderAPI.h"

namespace engine::render {

class IRenderBackend {
public:
  virtual ~IRenderBackend() = default;
  virtual GraphicsBackend backend() const = 0;
  virtual bool beginRenderPass(const RenderPassDescription& pass, std::string& error) = 0;
  virtual bool bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) = 0;
  virtual bool transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) = 0;
  virtual void endRenderPass() = 0;
};

class VulkanBackend final : public IRenderBackend {
public:
  GraphicsBackend backend() const override { return GraphicsBackend::Vulkan; }
  bool beginRenderPass(const RenderPassDescription& pass, std::string& error) override;
  bool bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) override;
  bool transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) override;
  void endRenderPass() override;

  const BackendRenderPassMapping& lastMapping() const { return mapping_; }

private:
  BackendRenderPassMapping mapping_;
};

class D3D12Backend final : public IRenderBackend {
public:
  GraphicsBackend backend() const override { return GraphicsBackend::D3D12; }
  bool beginRenderPass(const RenderPassDescription& pass, std::string& error) override;
  bool bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) override;
  bool transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) override;
  void endRenderPass() override;

private:
  std::string commandTrace_;
};

class MetalBackend final : public IRenderBackend {
public:
  GraphicsBackend backend() const override { return GraphicsBackend::Metal; }
  bool beginRenderPass(const RenderPassDescription& pass, std::string& error) override;
  bool bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) override;
  bool transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) override;
  void endRenderPass() override;

private:
  std::string encoderTrace_;
};


class OpenGlesBackend final : public IRenderBackend {
public:
  GraphicsBackend backend() const override { return GraphicsBackend::OpenGLES; }
  bool beginRenderPass(const RenderPassDescription& pass, std::string& error) override;
  bool bindGraphicsPipeline(const GraphicsPipelineDescription& pipeline, std::string& error) override;
  bool transitionResources(const std::vector<ResourceTransition>& transitions, std::string& error) override;
  void endRenderPass() override;
};

std::unique_ptr<IRenderBackend> createBackend(GraphicsBackend backend);

}  // namespace engine::render
