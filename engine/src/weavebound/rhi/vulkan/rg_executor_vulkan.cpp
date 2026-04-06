#include "weavebound/rhi/vulkan/rg_executor_vulkan.hpp"

#include <unordered_map>

namespace weavebound::rhi {

namespace {

rg::ResourceSurfaceKind surface_for(const rg::CompiledRenderGraph& crg, rg::ResourceId rid) {
  for (const auto& r : crg.resources) {
    if (r.id == rid) {
      return r.surface;
    }
  }
  return rg::ResourceSurfaceKind::Color;
}

const rg::PassNode* find_pass(const rg::CompiledRenderGraph& crg, rg::PassId id) {
  for (const auto& p : crg.passes) {
    if (p.id == id) {
      return &p;
    }
  }
  return nullptr;
}

const char* pass_name(const rg::CompiledRenderGraph& crg, rg::PassId id) {
  const rg::PassNode* p = find_pass(crg, id);
  return p ? p->name.c_str() : "pass";
}

}  // namespace

void record_render_graph_vulkan(VkCommandBuffer cmd, const rg::CompiledRenderGraph& crg,
                                PFN_vkCmdBeginDebugUtilsLabelEXT pfn_begin,
                                PFN_vkCmdEndDebugUtilsLabelEXT pfn_end,
                                VkImage (*resolve_image)(void* user, rg::ResourceId id), void* user) {
  std::unordered_map<rg::ResourceId, VkImageLayout> sim_layout;

  auto current_layout = [&](rg::ResourceId rid) -> VkImageLayout {
    const auto it = sim_layout.find(rid);
    return it == sim_layout.end() ? VK_IMAGE_LAYOUT_UNDEFINED : it->second;
  };

  for (rg::PassId pass : crg.pass_order) {
    for (const rg::BarrierRecord& b : crg.barriers) {
      if (b.consumer_pass != pass) {
        continue;
      }
      if (b.vk_src_stage_mask == 0u && b.vk_dst_stage_mask == 0u) {
        continue;
      }

      VkImage vk_img = VK_NULL_HANDLE;
      if (resolve_image) {
        vk_img = resolve_image(user, b.resource);
      }

      if (b.use_image_barrier && vk_img != VK_NULL_HANDLE) {
        const VkImageLayout old_layout = current_layout(b.resource);
        const VkImageLayout new_layout = static_cast<VkImageLayout>(b.vk_new_image_layout);

        VkImageMemoryBarrier imb{};
        imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imb.srcAccessMask = b.vk_src_access_mask;
        imb.dstAccessMask = b.vk_dst_access_mask;
        imb.oldLayout = old_layout;
        imb.newLayout = new_layout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.image = vk_img;
        imb.subresourceRange.aspectMask =
            b.vk_aspect_mask ? static_cast<VkImageAspectFlags>(b.vk_aspect_mask) : VK_IMAGE_ASPECT_COLOR_BIT;
        imb.subresourceRange.baseMipLevel = 0;
        imb.subresourceRange.levelCount = 1;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(cmd, b.vk_src_stage_mask, b.vk_dst_stage_mask, 0, 0, nullptr, 0, nullptr, 1,
                             &imb);
        sim_layout[b.resource] = new_layout;
      } else {
        VkMemoryBarrier mb{};
        mb.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        mb.srcAccessMask = b.vk_src_access_mask;
        mb.dstAccessMask = b.vk_dst_access_mask;
        vkCmdPipelineBarrier(cmd, b.vk_src_stage_mask, b.vk_dst_stage_mask, 0, 1, &mb, 0, nullptr, 0, nullptr);
      }
    }

    if (pfn_begin && pfn_end) {
      VkDebugUtilsLabelEXT lab{};
      lab.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
      lab.pLabelName = pass_name(crg, pass);
      lab.color[0] = 0.35f;
      lab.color[1] = 0.65f;
      lab.color[2] = 1.f;
      lab.color[3] = 1.f;
      pfn_begin(cmd, &lab);
      pfn_end(cmd);
    }

    const rg::PassNode* pn = find_pass(crg, pass);
    if (pn && pn->kind == rg::PassKind::Graphics) {
      for (rg::ResourceId wr : pn->writes) {
        VkImage img = VK_NULL_HANDLE;
        if (resolve_image) {
          img = resolve_image(user, wr);
        }
        if (img != VK_NULL_HANDLE) {
          sim_layout[wr] = surface_for(crg, wr) == rg::ResourceSurfaceKind::Depth
                              ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                              : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
      }
    }
  }
}

}  // namespace weavebound::rhi
