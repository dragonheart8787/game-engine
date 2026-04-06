#pragma once

#include <vulkan/vulkan.h>

#include "weavebound/render_graph/compiled.hpp"

namespace weavebound::rhi {

/**
 * 依 CompiledRenderGraph 錄製 pass 順序下的 barriers（VkImageMemoryBarrier 若可解析影像）與 debug labels。
 * `resolve_image(user, id)` 回傳 VK_NULL_HANDLE 時僅錄製 VkMemoryBarrier（與舊行為相容）。
 */
void record_render_graph_vulkan(VkCommandBuffer cmd, const rg::CompiledRenderGraph& crg,
                                PFN_vkCmdBeginDebugUtilsLabelEXT pfn_begin,
                                PFN_vkCmdEndDebugUtilsLabelEXT pfn_end,
                                VkImage (*resolve_image)(void* user, rg::ResourceId id), void* user);

}  // namespace weavebound::rhi
