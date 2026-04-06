#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "weavebound/render_graph/ir.hpp"
#include "weavebound/rhi/types.hpp"

namespace weavebound::rg {

class RenderGraphBuilder;

/**
 * Producer→consumer 之間的資源同步描述。
 * `vk_*` 欄位為 Vulkan 對齊的 stage/access mask（0 表示由執行層略過該 barrier）。
 */
struct BarrierRecord {
  PassId producer_pass{};
  PassId consumer_pass{};
  ResourceId resource{};
  rhi::ResourceState state_before{rhi::ResourceState::Undefined};
  rhi::ResourceState state_after{rhi::ResourceState::Undefined};
  std::uint32_t vk_src_stage_mask{0};
  std::uint32_t vk_dst_stage_mask{0};
  std::uint32_t vk_src_access_mask{0};
  std::uint32_t vk_dst_access_mask{0};
  /** 若為 true 且執行層能解析 `resource` 對應之 VkImage，則錄製 VkImageMemoryBarrier。 */
  bool use_image_barrier{false};
  /** Vulkan VkImageLayout 數值（見 rg_executor_vulkan / 編譯器填入）。 */
  std::uint32_t vk_old_image_layout{0};
  std::uint32_t vk_new_image_layout{0};
  /** Vulkan VkImageAspectFlags（例如 COLOR=1、DEPTH=2）；0 表示執行層用預設 COLOR。 */
  std::uint32_t vk_aspect_mask{0};
  std::string note;
};

struct CompiledRenderGraph {
  bool ok{true};
  std::string error;
  /** 編譯輸入快照（供 JSON dump / 除錯）。 */
  std::vector<ResourceNode> resources;
  std::vector<PassNode> passes;
  std::vector<PassId> pass_order;
  std::vector<BarrierRecord> barriers;

  std::string dump_json() const;
};

CompiledRenderGraph compile(const RenderGraphBuilder& builder);

}  // namespace weavebound::rg
