#include "engine/render/RenderAPI.h"

#include <sstream>

namespace engine::render {

namespace {
bool isDepthFormat(TextureFormat format) {
  return format == TextureFormat::D24S8 || format == TextureFormat::D32_FLOAT;
}

std::string stateToString(ResourceState state) {
  switch (state) {
    case ResourceState::Undefined:
      return "Undefined";
    case ResourceState::RenderTarget:
      return "RenderTarget";
    case ResourceState::DepthWrite:
      return "DepthWrite";
    case ResourceState::ShaderRead:
      return "ShaderRead";
    case ResourceState::ShaderWrite:
      return "ShaderWrite";
    case ResourceState::CopySrc:
      return "CopySrc";
    case ResourceState::CopyDst:
      return "CopyDst";
    case ResourceState::Present:
      return "Present";
  }
  return "Unknown";
}
}  // namespace

bool RenderApiContract::validateRenderPass(const RenderPassDescription& desc, std::string& error) {
  if (desc.attachments.empty()) {
    error = "RenderPass must contain at least one attachment";
    return false;
  }
  if (desc.subpasses.empty()) {
    error = "RenderPass must contain at least one subpass";
    return false;
  }

  for (std::size_t i = 0; i < desc.attachments.size(); ++i) {
    const auto& attachment = desc.attachments[i];
    if (attachment.format == TextureFormat::Unknown) {
      error = "Attachment[" + std::to_string(i) + "] format is Unknown";
      return false;
    }
  }

  for (std::size_t s = 0; s < desc.subpasses.size(); ++s) {
    const auto& subpass = desc.subpasses[s];
    for (std::uint32_t color : subpass.colorAttachments) {
      if (color >= desc.attachments.size()) {
        error = "Subpass[" + std::to_string(s) + "] color attachment out of range";
        return false;
      }
      if (isDepthFormat(desc.attachments[color].format)) {
        error = "Subpass[" + std::to_string(s) + "] color attachment points to depth format";
        return false;
      }
    }
    if (subpass.depthAttachment >= 0) {
      std::size_t depth = static_cast<std::size_t>(subpass.depthAttachment);
      if (depth >= desc.attachments.size()) {
        error = "Subpass[" + std::to_string(s) + "] depth attachment out of range";
        return false;
      }
      if (!isDepthFormat(desc.attachments[depth].format)) {
        error = "Subpass[" + std::to_string(s) + "] depth attachment is not a depth format";
        return false;
      }
    }
  }

  return true;
}

bool RenderApiContract::validatePipeline(const GraphicsPipelineDescription& desc, std::string& error) {
  if (desc.shader.vsPath.empty() && desc.shader.csPath.empty()) {
    error = "Pipeline must provide VS or CS shader";
    return false;
  }
  if (desc.shader.csPath.empty() && desc.shader.psPath.empty()) {
    error = "Graphics pipeline must provide PS shader when CS is absent";
    return false;
  }
  if (desc.renderTargetCount == 0 && desc.depthStencilFormat == TextureFormat::Unknown) {
    error = "Pipeline must output to at least one RT or depth target";
    return false;
  }
  if (desc.renderTargetCount > desc.renderTargetFormats.size()) {
    error = "Pipeline render target count exceeds array size";
    return false;
  }

  for (std::uint32_t i = 0; i < desc.renderTargetCount; ++i) {
    if (desc.renderTargetFormats[i] == TextureFormat::Unknown) {
      error = "Pipeline render target format is Unknown at slot " + std::to_string(i);
      return false;
    }
    if (isDepthFormat(desc.renderTargetFormats[i])) {
      error = "Pipeline render target slot " + std::to_string(i) + " cannot be depth format";
      return false;
    }
  }

  if (desc.depthStencilFormat != TextureFormat::Unknown && !isDepthFormat(desc.depthStencilFormat)) {
    error = "DepthStencilFormat must be a depth format";
    return false;
  }

  return true;
}

bool RenderApiContract::validateTransitions(const std::vector<ResourceTransition>& transitions, std::string& error) {
  for (std::size_t i = 0; i < transitions.size(); ++i) {
    const auto& t = transitions[i];
    if (t.resourceId == 0) {
      error = "Transition[" + std::to_string(i) + "] has invalid resource id";
      return false;
    }
    if (t.before == t.after) {
      error = "Transition[" + std::to_string(i) + "] before/after state identical";
      return false;
    }
  }
  return true;
}

BackendRenderPassMapping RenderApiContract::buildMentalModel(
    const RenderPassDescription& pass,
    const GraphicsPipelineDescription& pipeline) {
  BackendRenderPassMapping out;

  std::ostringstream vk;
  vk << "VkRenderPass(" << pass.name << ") with " << pass.attachments.size() << " attachments";
  out.vkRenderPass = vk.str();

  for (std::size_t i = 0; i < pass.attachments.size(); ++i) {
    std::ostringstream line;
    line << "AttachmentDescription[" << i << "] load/store with initial="
         << stateToString(pass.attachments[i].initialState) << " final="
         << stateToString(pass.attachments[i].finalState);
    out.vkAttachmentDescriptions.push_back(line.str());
  }

  for (std::size_t i = 0; i < pass.subpasses.size(); ++i) {
    out.vkSubpasses.push_back("Subpass[" + std::to_string(i) + "] color/depth refs configured");
  }

  out.d3d12OmSetRenderTargets = "OMSetRenderTargets with " + std::to_string(pipeline.renderTargetCount) + " RTVs";
  out.d3d12BeginRenderPass = "BeginRenderPass (future path) mirrors load/store semantics";
  out.d3d12ResourceTransitions.push_back("ResourceBarrier Transition for RTV/DSV/SRV states");

  out.metalRenderPassDescriptor = "MTLRenderPassDescriptor from attachment load/store actions";
  out.metalPipelineState = "MTLRenderPipelineState + MTLDepthStencilState from pipeline description";

  return out;
}

}  // namespace engine::render
