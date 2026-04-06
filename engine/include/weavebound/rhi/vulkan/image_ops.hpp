#pragma once

#include <vulkan/vulkan.h>

namespace weavebound::rhi::vulkan {

/** 單次影像記憶體 barrier（Lit / RG 執行層共用）。 */
void cmd_image_barrier(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout old_layout,
                       VkImageLayout new_layout, VkAccessFlags src_access, VkAccessFlags dst_access,
                       VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

}  // namespace weavebound::rhi::vulkan
