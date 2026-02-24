#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::render {

enum class GraphicsBackend {
  Vulkan,
  D3D12,
  Metal,
  OpenGLES
};

enum class TextureFormat {
  Unknown,
  RGBA8_UNORM,
  BGRA8_UNORM,
  D24S8,
  D32_FLOAT
};

enum class LoadOp {
  Load,
  Clear,
  DontCare
};

enum class StoreOp {
  Store,
  DontCare
};

enum class Topology {
  TriangleList,
  TriangleStrip,
  LineList,
  PointList
};

enum class CullMode {
  None,
  Front,
  Back
};

enum class FillMode {
  Solid,
  Wireframe
};

enum class CompareOp {
  Never,
  Less,
  LessEqual,
  Equal,
  Greater,
  Always
};

enum class BlendFactor {
  Zero,
  One,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha
};

enum class BlendOp {
  Add,
  Subtract,
  ReverseSubtract,
  Min,
  Max
};

enum class ResourceState {
  Undefined,
  RenderTarget,
  DepthWrite,
  ShaderRead,
  ShaderWrite,
  CopySrc,
  CopyDst,
  Present
};

enum class ShaderStage {
  VS,
  PS,
  CS
};

struct AttachmentDescription {
  TextureFormat format = TextureFormat::Unknown;
  LoadOp loadOp = LoadOp::Load;
  StoreOp storeOp = StoreOp::Store;
  ResourceState initialState = ResourceState::Undefined;
  ResourceState finalState = ResourceState::Present;
};

struct SubpassDescription {
  std::vector<std::uint32_t> colorAttachments;
  std::int32_t depthAttachment = -1;
  std::vector<std::uint32_t> inputAttachments;
};

struct RenderPassDescription {
  std::string name;
  std::vector<AttachmentDescription> attachments;
  std::vector<SubpassDescription> subpasses;
};

struct ShaderProgram {
  std::string vsPath;
  std::string psPath;
  std::string csPath;
};

struct InputElement {
  std::string semantic;
  std::uint32_t location = 0;
  std::uint32_t offset = 0;
};

struct BlendState {
  bool enabled = false;
  BlendFactor srcColor = BlendFactor::One;
  BlendFactor dstColor = BlendFactor::Zero;
  BlendOp colorOp = BlendOp::Add;
};

struct DepthState {
  bool depthTestEnabled = true;
  bool depthWriteEnabled = true;
  CompareOp compareOp = CompareOp::LessEqual;
};

struct RasterizerState {
  CullMode cullMode = CullMode::Back;
  FillMode fillMode = FillMode::Solid;
  bool frontCounterClockwise = false;
};

struct GraphicsPipelineDescription {
  std::string name;
  ShaderProgram shader;
  std::vector<InputElement> inputLayout;
  BlendState blend;
  DepthState depth;
  RasterizerState rasterizer;
  Topology topology = Topology::TriangleList;
  std::array<TextureFormat, 8> renderTargetFormats{};
  std::uint32_t renderTargetCount = 0;
  TextureFormat depthStencilFormat = TextureFormat::Unknown;
};

struct ResourceTransition {
  std::uint64_t resourceId = 0;
  ResourceState before = ResourceState::Undefined;
  ResourceState after = ResourceState::Undefined;
};

struct BackendRenderPassMapping {
  // Vulkan mental model.
  std::string vkRenderPass;
  std::vector<std::string> vkSubpasses;
  std::vector<std::string> vkAttachmentDescriptions;

  // D3D12 mental model.
  std::string d3d12BeginRenderPass;
  std::string d3d12OmSetRenderTargets;
  std::vector<std::string> d3d12ResourceTransitions;

  // Metal mental model.
  std::string metalRenderPassDescriptor;
  std::string metalPipelineState;
};

class RenderApiContract {
public:
  static bool validateRenderPass(const RenderPassDescription& desc, std::string& error);
  static bool validatePipeline(const GraphicsPipelineDescription& desc, std::string& error);
  static bool validateTransitions(const std::vector<ResourceTransition>& transitions, std::string& error);

  static BackendRenderPassMapping buildMentalModel(
      const RenderPassDescription& pass,
      const GraphicsPipelineDescription& pipeline);
};

}  // namespace engine::render
