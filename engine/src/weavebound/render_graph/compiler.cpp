#include "weavebound/render_graph/compiled.hpp"

#include "weavebound/render_graph/builder.hpp"

#include <cstdio>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace weavebound::rg {

namespace {

// 與 Vulkan 1.x 常數對齊（此編譯單元不 include vulkan.hpp，避免 RG 依賴 RHI 後端）。
constexpr std::uint32_t kVkPipelineStageTopOfPipe = 0x00000001u;
constexpr std::uint32_t kVkPipelineStageFragmentShader = 0x00000080u;
constexpr std::uint32_t kVkPipelineStageColorAttachmentOutput = 0x00000400u;
constexpr std::uint32_t kVkPipelineStageComputeShader = 0x00000800u;
constexpr std::uint32_t kVkPipelineStageBottomOfPipe = 0x00000200u;
constexpr std::uint32_t kVkAccessShaderRead = 0x00000020u;
constexpr std::uint32_t kVkAccessShaderWrite = 0x00000040u;
constexpr std::uint32_t kVkAccessColorAttachmentWrite = 0x00000100u;
constexpr std::uint32_t kVkImageLayoutGeneral = 1u;
constexpr std::uint32_t kVkImageLayoutColorAttachmentOptimal = 2u;
constexpr std::uint32_t kVkImageLayoutDepthStencilAttachmentOptimal = 3u;
constexpr std::uint32_t kVkImageLayoutDepthStencilReadOnlyOptimal = 4u;
constexpr std::uint32_t kVkImageLayoutShaderReadOnlyOptimal = 5u;
constexpr std::uint32_t kVkImageLayoutTransferDstOptimal = 7u;
constexpr std::uint32_t kVkPipelineStageLateFragmentTests = 0x00000200u;
constexpr std::uint32_t kVkAccessDepthStencilAttachmentWrite = 0x00000400u;
constexpr std::uint32_t kVkImageAspectColor = 0x00000001u;
constexpr std::uint32_t kVkImageAspectDepth = 0x00000002u;

const char* pass_kind_str(PassKind k) {
  switch (k) {
    case PassKind::Graphics:
      return "Graphics";
    case PassKind::Compute:
      return "Compute";
    case PassKind::Copy:
      return "Copy";
    case PassKind::Resolve:
      return "Resolve";
  }
  return "Graphics";
}

const char* resource_surface_str(ResourceSurfaceKind s) {
  return s == ResourceSurfaceKind::Depth ? "Depth" : "Color";
}

const char* resource_state_str(rhi::ResourceState s) {
  switch (s) {
    case rhi::ResourceState::Undefined:
      return "Undefined";
    case rhi::ResourceState::Common:
      return "Common";
    case rhi::ResourceState::RenderTarget:
      return "RenderTarget";
    case rhi::ResourceState::Present:
      return "Present";
    case rhi::ResourceState::ShaderResource:
      return "ShaderResource";
    case rhi::ResourceState::Storage:
      return "Storage";
    case rhi::ResourceState::CopyDest:
      return "CopyDest";
    case rhi::ResourceState::CopySrc:
      return "CopySrc";
  }
  return "Undefined";
}

ResourceSurfaceKind surface_for_resource(ResourceId rid, const std::vector<ResourceNode>& nodes) {
  for (const auto& n : nodes) {
    if (n.id == rid) {
      return n.surface;
    }
  }
  return ResourceSurfaceKind::Color;
}

void fill_barrier_from_passes(const PassNode& producer, const PassNode& consumer, ResourceSurfaceKind res_surface,
                              BarrierRecord& br) {
  const bool prod_gfx = producer.kind == PassKind::Graphics;
  const bool prod_cmp = producer.kind == PassKind::Compute;
  const bool cons_gfx = consumer.kind == PassKind::Graphics;
  const bool cons_cmp = consumer.kind == PassKind::Compute;

  if (res_surface == ResourceSurfaceKind::Depth && prod_gfx && (cons_gfx || cons_cmp)) {
    br.state_before = rhi::ResourceState::RenderTarget;
    br.state_after = rhi::ResourceState::ShaderResource;
    br.vk_src_stage_mask = kVkPipelineStageLateFragmentTests;
    br.vk_dst_stage_mask = cons_cmp ? kVkPipelineStageComputeShader : kVkPipelineStageFragmentShader;
    br.vk_src_access_mask = kVkAccessDepthStencilAttachmentWrite;
    br.vk_dst_access_mask = kVkAccessShaderRead;
    br.use_image_barrier = true;
    br.vk_old_image_layout = kVkImageLayoutDepthStencilAttachmentOptimal;
    br.vk_new_image_layout = kVkImageLayoutDepthStencilReadOnlyOptimal;
    br.vk_aspect_mask = kVkImageAspectDepth;
    br.note = "depth_attachment_to_srv";
  } else if (prod_gfx && (cons_gfx || cons_cmp)) {
    br.state_before = rhi::ResourceState::RenderTarget;
    br.state_after = rhi::ResourceState::ShaderResource;
    br.vk_src_stage_mask = kVkPipelineStageColorAttachmentOutput;
    br.vk_dst_stage_mask = cons_cmp ? kVkPipelineStageComputeShader : kVkPipelineStageFragmentShader;
    br.vk_src_access_mask = kVkAccessColorAttachmentWrite;
    br.vk_dst_access_mask = kVkAccessShaderRead;
    br.use_image_barrier = true;
    br.vk_old_image_layout = kVkImageLayoutColorAttachmentOptimal;
    br.vk_new_image_layout = kVkImageLayoutShaderReadOnlyOptimal;
    br.vk_aspect_mask = kVkImageAspectColor;
    br.note = "color_attachment_to_srv";
  } else if (prod_cmp && (cons_gfx || cons_cmp)) {
    br.state_before = rhi::ResourceState::Storage;
    br.state_after = rhi::ResourceState::ShaderResource;
    br.vk_src_stage_mask = kVkPipelineStageComputeShader;
    br.vk_dst_stage_mask = cons_gfx ? kVkPipelineStageFragmentShader : kVkPipelineStageComputeShader;
    br.vk_src_access_mask = kVkAccessShaderWrite;
    br.vk_dst_access_mask = kVkAccessShaderRead;
    br.use_image_barrier = true;
    br.vk_old_image_layout = kVkImageLayoutGeneral;
    br.vk_new_image_layout = kVkImageLayoutShaderReadOnlyOptimal;
    br.vk_aspect_mask = kVkImageAspectColor;
    br.note = "uav_to_srv";
  } else if (producer.kind == PassKind::Copy && (cons_gfx || cons_cmp)) {
    br.state_before = rhi::ResourceState::CopyDest;
    br.state_after = rhi::ResourceState::ShaderResource;
    br.vk_src_stage_mask = kVkPipelineStageBottomOfPipe;
    br.vk_dst_stage_mask = cons_cmp ? kVkPipelineStageComputeShader : kVkPipelineStageFragmentShader;
    br.vk_src_access_mask = kVkAccessShaderWrite;
    br.vk_dst_access_mask = kVkAccessShaderRead;
    br.use_image_barrier = true;
    br.vk_old_image_layout = kVkImageLayoutTransferDstOptimal;
    br.vk_new_image_layout = kVkImageLayoutShaderReadOnlyOptimal;
    br.vk_aspect_mask = kVkImageAspectColor;
    br.note = "copy_to_srv";
  } else {
    br.state_before = rhi::ResourceState::Common;
    br.state_after = rhi::ResourceState::Common;
    br.vk_src_stage_mask = kVkPipelineStageBottomOfPipe;
    br.vk_dst_stage_mask = kVkPipelineStageTopOfPipe;
    br.vk_src_access_mask = 0;
    br.vk_dst_access_mask = 0;
    br.use_image_barrier = false;
    br.vk_aspect_mask = 0;
    br.note = "fallback_noop";
  }
}

void append_escaped_json_string(std::string& out, std::string_view s) {
  out.push_back('"');
  for (char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out += buf;
        } else {
          out.push_back(c);
        }
    }
  }
  out.push_back('"');
}

}  // namespace

CompiledRenderGraph compile(const RenderGraphBuilder& builder) {
  CompiledRenderGraph out;
  out.resources = builder.resources();
  out.passes = builder.passes();
  const auto& passes = out.passes;
  const auto n = passes.size();
  if (n == 0) {
    out.pass_order.clear();
    out.barriers.clear();
    return out;
  }

  std::unordered_map<ResourceId, PassId> last_writer;
  std::vector<std::vector<PassId>> adj(n);
  std::vector<std::uint32_t> indeg(n, 0);
  std::vector<BarrierRecord> barriers;
  std::unordered_set<std::uint64_t> pass_edges;

  for (PassId i = 0; i < static_cast<PassId>(n); ++i) {
    const auto& p = passes[i];
    for (ResourceId r : p.reads) {
      auto it = last_writer.find(r);
      if (it != last_writer.end() && it->second != i) {
        const PassId u = it->second;
        const std::uint64_t ekey =
            (static_cast<std::uint64_t>(u) << 32) | static_cast<std::uint64_t>(i);
        if (pass_edges.insert(ekey).second) {
          adj[u].push_back(i);
          indeg[i]++;
        }
        BarrierRecord br;
        br.producer_pass = u;
        br.consumer_pass = i;
        br.resource = r;
        fill_barrier_from_passes(passes[u], passes[i], surface_for_resource(r, out.resources), br);
        barriers.push_back(std::move(br));
      }
    }
    for (ResourceId r : p.writes) {
      last_writer[r] = i;
    }
  }

  std::queue<PassId> q;
  for (PassId i = 0; i < static_cast<PassId>(n); ++i) {
    if (indeg[i] == 0) {
      q.push(i);
    }
  }

  std::vector<PassId> order;
  order.reserve(n);
  while (!q.empty()) {
    const PassId u = q.front();
    q.pop();
    order.push_back(u);
    for (PassId v : adj[u]) {
      if (--indeg[v] == 0) {
        q.push(v);
      }
    }
  }

  if (order.size() != n) {
    out.ok = false;
    out.error = "render_graph: cycle detected in pass dependencies";
    out.pass_order.clear();
    out.barriers.clear();
    return out;
  }

  out.pass_order = std::move(order);
  out.barriers = std::move(barriers);
  return out;
}

std::string CompiledRenderGraph::dump_json() const {
  std::string j;
  j.reserve(1024);
  j += "{\"schema\":1,\"ok\":";
  j += ok ? "true" : "false";
  if (!ok) {
    j += ",\"error\":";
    append_escaped_json_string(j, error);
  }
  j += ",\"resources\":[";
  for (std::size_t i = 0; i < resources.size(); ++i) {
    if (i) {
      j += ',';
    }
    const auto& r = resources[i];
    char idb[32];
    std::snprintf(idb, sizeof(idb), "%u", static_cast<unsigned>(r.id));
    j += "{\"id\":";
    j += idb;
    j += ",\"name\":";
    append_escaped_json_string(j, r.name);
    j += ",\"transient\":";
    j += r.transient ? "true" : "false";
    j += ",\"surface\":\"";
    j += resource_surface_str(r.surface);
    j += "\"}";
  }
  j += "],\"passes\":[";
  for (std::size_t i = 0; i < passes.size(); ++i) {
    if (i) {
      j += ',';
    }
    const auto& p = passes[i];
    char idb[32];
    std::snprintf(idb, sizeof(idb), "%u", static_cast<unsigned>(p.id));
    j += "{\"id\":";
    j += idb;
    j += ",\"name\":";
    append_escaped_json_string(j, p.name);
    j += ",\"kind\":\"";
    j += pass_kind_str(p.kind);
    j += "\"}";
  }
  j += "],\"pass_order\":[";
  for (std::size_t i = 0; i < pass_order.size(); ++i) {
    if (i) {
      j += ',';
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(pass_order[i]));
    j += buf;
  }
  j += "],\"barriers\":[";
  for (std::size_t i = 0; i < barriers.size(); ++i) {
    if (i) {
      j += ',';
    }
    const auto& b = barriers[i];
    char pb[32], cb[32], rb[32];
    std::snprintf(pb, sizeof(pb), "%u", static_cast<unsigned>(b.producer_pass));
    std::snprintf(cb, sizeof(cb), "%u", static_cast<unsigned>(b.consumer_pass));
    std::snprintf(rb, sizeof(rb), "%u", static_cast<unsigned>(b.resource));
    j += "{\"producer_pass\":";
    j += pb;
    j += ",\"consumer_pass\":";
    j += cb;
    j += ",\"resource\":";
    j += rb;
    j += ",\"state_before\":\"";
    j += resource_state_str(b.state_before);
    j += "\",\"state_after\":\"";
    j += resource_state_str(b.state_after);
    j += "\",\"vk_src_stage_mask\":";
    char sm[32];
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_src_stage_mask));
    j += sm;
    j += ",\"vk_dst_stage_mask\":";
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_dst_stage_mask));
    j += sm;
    j += ",\"vk_src_access_mask\":";
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_src_access_mask));
    j += sm;
    j += ",\"vk_dst_access_mask\":";
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_dst_access_mask));
    j += sm;
    j += ",\"use_image_barrier\":";
    j += b.use_image_barrier ? "true" : "false";
    j += ",\"vk_old_image_layout\":";
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_old_image_layout));
    j += sm;
    j += ",\"vk_new_image_layout\":";
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_new_image_layout));
    j += sm;
    j += ",\"vk_aspect_mask\":";
    std::snprintf(sm, sizeof(sm), "%u", static_cast<unsigned>(b.vk_aspect_mask));
    j += sm;
    j += ",\"note\":";
    append_escaped_json_string(j, b.note);
    j += '}';
  }
  j += "]}";
  return j;
}

}  // namespace weavebound::rg
