#include "weavebound/rhi/vulkan/image_ops.hpp"

namespace weavebound::rhi::vulkan {

void cmd_image_barrier(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout old_layout,
                       VkImageLayout new_layout, VkAccessFlags src_access, VkAccessFlags dst_access,
                       VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage) {
  VkImageMemoryBarrier b{};
  b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  b.srcAccessMask = src_access;
  b.dstAccessMask = dst_access;
  b.oldLayout = old_layout;
  b.newLayout = new_layout;
  b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  b.image = image;
  b.subresourceRange.aspectMask = aspect;
  b.subresourceRange.levelCount = 1;
  b.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &b);
}

}  // namespace weavebound::rhi::vulkan
