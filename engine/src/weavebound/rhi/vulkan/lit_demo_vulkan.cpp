#include "weavebound/rhi/lit_demo_vulkan.hpp"
#include "weavebound/rhi/vulkan/image_ops.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace weavebound::rhi {

namespace {

std::vector<char> read_spv(const std::filesystem::path& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    return {};
  }
  const auto sz = static_cast<size_t>(f.tellg());
  if (sz == 0 || (sz % 4) != 0) {
    return {};
  }
  std::vector<char> buf(sz);
  f.seekg(0);
  f.read(buf.data(), static_cast<std::streamsize>(sz));
  return buf;
}

std::uint32_t find_mem_type(VkPhysicalDevice phys, std::uint32_t bits, VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties mp{};
  vkGetPhysicalDeviceMemoryProperties(phys, &mp);
  for (std::uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
    if ((bits & (1u << i)) != 0 && (mp.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  return UINT32_MAX;
}

struct GImg {
  VkImage img{};
  VkDeviceMemory mem{};
  VkImageView view{};
  VkExtent2D ext{};
  VkFormat fmt{VK_FORMAT_UNDEFINED};
};

void destroy_gimg(VkDevice d, GImg& g) {
  if (g.view) {
    vkDestroyImageView(d, g.view, nullptr);
  }
  if (g.img) {
    vkDestroyImage(d, g.img, nullptr);
  }
  if (g.mem) {
    vkFreeMemory(d, g.mem, nullptr);
  }
  g = {};
}

bool mk_image2d(VkDevice device, VkPhysicalDevice phys, std::uint32_t w, std::uint32_t h, VkFormat fmt,
                VkImageUsageFlags use, VkImageAspectFlags asp, GImg& out) {
  VkImageCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent = {w, h, 1};
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = use;
  ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(device, &ici, nullptr, &out.img) != VK_SUCCESS) {
    return false;
  }
  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device, out.img, &req);
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = find_mem_type(phys, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mai.memoryTypeIndex == UINT32_MAX) {
    destroy_gimg(device, out);
    return false;
  }
  if (vkAllocateMemory(device, &mai, nullptr, &out.mem) != VK_SUCCESS) {
    destroy_gimg(device, out);
    return false;
  }
  vkBindImageMemory(device, out.img, out.mem, 0);
  VkImageViewCreateInfo iv{};
  iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  iv.image = out.img;
  iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
  iv.format = fmt;
  iv.subresourceRange.aspectMask = asp;
  iv.subresourceRange.levelCount = 1;
  iv.subresourceRange.layerCount = 1;
  if (vkCreateImageView(device, &iv, nullptr, &out.view) != VK_SUCCESS) {
    destroy_gimg(device, out);
    return false;
  }
  out.ext = {w, h};
  out.fmt = fmt;
  return true;
}

bool mk_buffer(VkDevice d, VkPhysicalDevice phys, VkDeviceSize sz, VkBufferUsageFlags u,
               VkMemoryPropertyFlags memp, VkBuffer& buf, VkDeviceMemory& mem) {
  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = sz;
  bci.usage = u;
  if (vkCreateBuffer(d, &bci, nullptr, &buf) != VK_SUCCESS) {
    return false;
  }
  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(d, buf, &req);
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = find_mem_type(phys, req.memoryTypeBits, memp);
  if (mai.memoryTypeIndex == UINT32_MAX) {
    vkDestroyBuffer(d, buf, nullptr);
    return false;
  }
  if (vkAllocateMemory(d, &mai, nullptr, &mem) != VK_SUCCESS) {
    vkDestroyBuffer(d, buf, nullptr);
    return false;
  }
  vkBindBufferMemory(d, buf, mem, 0);
  return true;
}

void mat_mul(const float a[16], const float b[16], float o[16]) {
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      o[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] + a[1 * 4 + r] * b[c * 4 + 1] + a[2 * 4 + r] * b[c * 4 + 2] +
                     a[3 * 4 + r] * b[c * 4 + 3];
    }
  }
}

void mat_id(float m[16]) {
  std::memset(m, 0, 64);
  m[0] = m[5] = m[10] = m[15] = 1.f;
}

void mat_persp(float fovy_deg, float aspect, float n, float f, float o[16]) {
  const float rad = fovy_deg * 3.14159265f / 180.f;
  const float t = n * std::tan(rad * 0.5f);
  const float r = t * aspect;
  const float l = -r;
  const float b = -t;
  std::memset(o, 0, 64);
  o[0] = 2.f * n / (r - l);
  o[5] = 2.f * n / (t - b);
  o[8] = (r + l) / (r - l);
  o[9] = (t + b) / (t - b);
  o[10] = -(f + n) / (f - n);
  o[11] = -1.f;
  o[14] = -(2.f * f * n) / (f - n);
}

void mat_look(float eye[3], float at[3], float upv[3], float o[16]) {
  float f[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
  float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
  if (fl > 1e-6f) {
    f[0] /= fl;
    f[1] /= fl;
    f[2] /= fl;
  }
  float s[3] = {f[1] * upv[2] - f[2] * upv[1], f[2] * upv[0] - f[0] * upv[2], f[0] * upv[1] - f[1] * upv[0]};
  fl = std::sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
  if (fl > 1e-6f) {
    s[0] /= fl;
    s[1] /= fl;
    s[2] /= fl;
  }
  float u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0]};
  o[0] = s[0];
  o[1] = u[0];
  o[2] = -f[0];
  o[3] = 0;
  o[4] = s[1];
  o[5] = u[1];
  o[6] = -f[1];
  o[7] = 0;
  o[8] = s[2];
  o[9] = u[2];
  o[10] = -f[2];
  o[11] = 0;
  o[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
  o[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
  o[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];
  o[15] = 1.f;
}

void mat_ortho(float l, float r, float b, float t, float n, float f, float o[16]) {
  std::memset(o, 0, 64);
  o[0] = 2.f / (r - l);
  o[5] = 2.f / (t - b);
  o[10] = -1.f / (f - n);
  o[12] = -(r + l) / (r - l);
  o[13] = -(t + b) / (t - b);
  o[14] = -n / (f - n);
  o[15] = 1.f;
}

}  // namespace

struct LitDemoRecorder::Impl {
  LitDemoInitCtx ctx{};
  bool ready{false};
  VkExtent2D ext{};

  std::filesystem::path shader_dir;

  GImg shadow{};
  VkRenderPass shadow_rp{};
  VkFramebuffer shadow_fb{};
  VkPipeline shadow_pipe{};
  VkPipelineLayout shadow_pl{};
  VkShaderModule shadow_vs{};
  VkShaderModule shadow_fs{};

  VkFormat hdr_fmt{VK_FORMAT_R16G16B16A16_SFLOAT};
  GImg hdr{};
  GImg hdr_depth{};
  VkRenderPass hdr_rp{};
  VkFramebuffer hdr_fb{};

  VkPipeline lit_pipe{};
  VkPipelineLayout lit_pl{};
  VkShaderModule lit_vs{};
  VkShaderModule lit_fs{};

  VkSampler shadow_samp{};
  VkDescriptorSetLayout lit_dsl{};
  VkDescriptorPool dpool{};
  VkDescriptorSet lit_set{};

  VkPipeline comp_pipe{};
  VkPipelineLayout comp_pl{};
  VkShaderModule fsq_vs{};
  VkShaderModule comp_fs{};
  VkDescriptorSetLayout comp_dsl{};
  VkDescriptorSet comp_set{};

  VkBuffer vb{};
  VkBuffer ib{};
  VkDeviceMemory vb_mem{};
  VkDeviceMemory ib_mem{};
  std::uint32_t idx_count{};

  VkBuffer ubo{};
  VkDeviceMemory ubo_mem{};
  void* ubo_map{};

  VkCommandPool up_pool{};
  VkCommandBuffer up_cmd{};
  VkFence up_fence{};

  VkImageLayout shadow_layout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkImageLayout hdr_color_layout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkImageLayout hdr_depth_layout{VK_IMAGE_LAYOUT_UNDEFINED};

  VkDescriptorPool imgui_pool{VK_NULL_HANDLE};
  bool imgui_inited{false};

  void nukem(VkDevice d) {
    if (imgui_inited) {
      ImGui_ImplVulkan_Shutdown();
      ImGui_ImplWin32_Shutdown();
      ImGui::DestroyContext();
      imgui_inited = false;
    }
    if (imgui_pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(d, imgui_pool, nullptr);
      imgui_pool = VK_NULL_HANDLE;
    }
    if (comp_pipe) {
      vkDestroyPipeline(d, comp_pipe, nullptr);
      comp_pipe = VK_NULL_HANDLE;
    }
    if (lit_pipe) {
      vkDestroyPipeline(d, lit_pipe, nullptr);
      lit_pipe = VK_NULL_HANDLE;
    }
    if (shadow_pipe) {
      vkDestroyPipeline(d, shadow_pipe, nullptr);
      shadow_pipe = VK_NULL_HANDLE;
    }
    for (VkShaderModule* m : {&comp_fs, &fsq_vs, &lit_fs, &lit_vs, &shadow_fs, &shadow_vs}) {
      if (*m) {
        vkDestroyShaderModule(d, *m, nullptr);
        *m = VK_NULL_HANDLE;
      }
    }
    if (comp_pl) {
      vkDestroyPipelineLayout(d, comp_pl, nullptr);
      comp_pl = VK_NULL_HANDLE;
    }
    if (lit_pl) {
      vkDestroyPipelineLayout(d, lit_pl, nullptr);
      lit_pl = VK_NULL_HANDLE;
    }
    if (shadow_pl) {
      vkDestroyPipelineLayout(d, shadow_pl, nullptr);
      shadow_pl = VK_NULL_HANDLE;
    }
    if (hdr_fb) {
      vkDestroyFramebuffer(d, hdr_fb, nullptr);
      hdr_fb = VK_NULL_HANDLE;
    }
    if (hdr_rp) {
      vkDestroyRenderPass(d, hdr_rp, nullptr);
      hdr_rp = VK_NULL_HANDLE;
    }
    destroy_gimg(d, hdr);
    destroy_gimg(d, hdr_depth);
    if (shadow_fb) {
      vkDestroyFramebuffer(d, shadow_fb, nullptr);
      shadow_fb = VK_NULL_HANDLE;
    }
    if (shadow_rp) {
      vkDestroyRenderPass(d, shadow_rp, nullptr);
      shadow_rp = VK_NULL_HANDLE;
    }
    destroy_gimg(d, shadow);
    if (lit_set) {
      lit_set = VK_NULL_HANDLE;
    }
    if (comp_set) {
      comp_set = VK_NULL_HANDLE;
    }
    if (dpool) {
      vkDestroyDescriptorPool(d, dpool, nullptr);
      dpool = VK_NULL_HANDLE;
    }
    if (comp_dsl) {
      vkDestroyDescriptorSetLayout(d, comp_dsl, nullptr);
      comp_dsl = VK_NULL_HANDLE;
    }
    if (lit_dsl) {
      vkDestroyDescriptorSetLayout(d, lit_dsl, nullptr);
      lit_dsl = VK_NULL_HANDLE;
    }
    if (shadow_samp) {
      vkDestroySampler(d, shadow_samp, nullptr);
      shadow_samp = VK_NULL_HANDLE;
    }
    if (vb) {
      vkDestroyBuffer(d, vb, nullptr);
      vb = VK_NULL_HANDLE;
    }
    if (ib) {
      vkDestroyBuffer(d, ib, nullptr);
      ib = VK_NULL_HANDLE;
    }
    if (vb_mem) {
      vkFreeMemory(d, vb_mem, nullptr);
      vb_mem = VK_NULL_HANDLE;
    }
    if (ib_mem) {
      vkFreeMemory(d, ib_mem, nullptr);
      ib_mem = VK_NULL_HANDLE;
    }
    if (ubo) {
      vkDestroyBuffer(d, ubo, nullptr);
      ubo = VK_NULL_HANDLE;
    }
    if (ubo_mem) {
      vkFreeMemory(d, ubo_mem, nullptr);
      ubo_mem = VK_NULL_HANDLE;
    }
    ubo_map = nullptr;
    if (up_fence) {
      vkDestroyFence(d, up_fence, nullptr);
      up_fence = VK_NULL_HANDLE;
    }
    if (up_pool) {
      vkDestroyCommandPool(d, up_pool, nullptr);
      up_pool = VK_NULL_HANDLE;
      up_cmd = VK_NULL_HANDLE;
    }
    ready = false;
  }

  bool pick_hdr_format(VkPhysicalDevice phys) {
    VkFormat want[] = {VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_B8G8R8A8_UNORM};
    for (VkFormat f : want) {
      VkFormatProperties p{};
      vkGetPhysicalDeviceFormatProperties(phys, f, &p);
      if ((p.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) != 0) {
        hdr_fmt = f;
        return true;
      }
    }
    return false;
  }

  bool build_hdr_pass() {
    VkDevice d = ctx.device;
    VkAttachmentDescription ca{};
    ca.format = hdr_fmt;
    ca.samples = VK_SAMPLE_COUNT_1_BIT;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ca.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ca.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentDescription da{};
    da.format = VK_FORMAT_D32_SFLOAT;
    da.samples = VK_SAMPLE_COUNT_1_BIT;
    da.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    da.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    da.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    da.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    da.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    da.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference cr{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference dr{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sd{};
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.colorAttachmentCount = 1;
    sd.pColorAttachments = &cr;
    sd.pDepthStencilAttachment = &dr;
    VkAttachmentDescription ads[2] = {ca, da};
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 2;
    rpci.pAttachments = ads;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sd;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    return vkCreateRenderPass(d, &rpci, nullptr, &hdr_rp) == VK_SUCCESS;
  }

  bool alloc_hdr_images() {
    VkDevice d = ctx.device;
    VkPhysicalDevice p = ctx.physical;
    if (!mk_image2d(d, p, ext.width, ext.height, hdr_fmt,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
                    hdr)) {
      return false;
    }
    if (!mk_image2d(d, p, ext.width, ext.height, VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, hdr_depth)) {
      destroy_gimg(d, hdr);
      return false;
    }
    VkImageView v[2] = {hdr.view, hdr_depth.view};
    VkFramebufferCreateInfo fbci{};
    fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.renderPass = hdr_rp;
    fbci.attachmentCount = 2;
    fbci.pAttachments = v;
    fbci.width = ext.width;
    fbci.height = ext.height;
    fbci.layers = 1;
    if (vkCreateFramebuffer(d, &fbci, nullptr, &hdr_fb) != VK_SUCCESS) {
      destroy_gimg(d, hdr);
      destroy_gimg(d, hdr_depth);
      return false;
    }
    return true;
  }

  void write_lit_desc() {
    VkDescriptorBufferInfo dbi{};
    dbi.buffer = ubo;
    dbi.offset = 0;
    dbi.range = 256;
    VkDescriptorImageInfo dii{};
    dii.sampler = shadow_samp;
    dii.imageView = shadow.view;
    dii.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet w[2]{};
    w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[0].dstSet = lit_set;
    w[0].dstBinding = 0;
    w[0].descriptorCount = 1;
    w[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w[0].pBufferInfo = &dbi;
    w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w[1].dstSet = lit_set;
    w[1].dstBinding = 1;
    w[1].descriptorCount = 1;
    w[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w[1].pImageInfo = &dii;
    vkUpdateDescriptorSets(ctx.device, 2, w, 0, nullptr);
  }

  void write_comp_desc() {
    VkDescriptorImageInfo dii{};
    dii.sampler = shadow_samp;
    dii.imageView = hdr.view;
    dii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = comp_set;
    w.dstBinding = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo = &dii;
    vkUpdateDescriptorSets(ctx.device, 1, &w, 0, nullptr);
  }

  bool create_shader_mod(VkDevice d, const std::vector<char>& code, VkShaderModule* out) {
    if (code.empty()) {
      return false;
    }
    VkShaderModuleCreateInfo sm{};
    sm.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm.codeSize = code.size();
    sm.pCode = reinterpret_cast<const std::uint32_t*>(code.data());
    return vkCreateShaderModule(d, &sm, nullptr, out) == VK_SUCCESS;
  }

  bool load_shaders_and_pipelines() {
    VkDevice d = ctx.device;
    const auto sd = shader_dir;
    if (!create_shader_mod(d, read_spv(sd / "shadow_depth.vert.spv"), &shadow_vs) ||
        !create_shader_mod(d, read_spv(sd / "shadow_depth.frag.spv"), &shadow_fs) ||
        !create_shader_mod(d, read_spv(sd / "lit_mesh.vert.spv"), &lit_vs) ||
        !create_shader_mod(d, read_spv(sd / "lit_mesh.frag.spv"), &lit_fs) ||
        !create_shader_mod(d, read_spv(sd / "fullscreen.vert.spv"), &fsq_vs) ||
        !create_shader_mod(d, read_spv(sd / "composite.frag.spv"), &comp_fs)) {
      return false;
    }

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcr.offset = 0;
    pcr.size = 64;
    VkPipelineLayoutCreateInfo pl{};
    pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(d, &pl, nullptr, &shadow_pl) != VK_SUCCESS) {
      return false;
    }

    VkPipelineLayoutCreateInfo pll{};
    pll.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pll.setLayoutCount = 1;
    pll.pSetLayouts = &lit_dsl;
    pll.pushConstantRangeCount = 1;
    pll.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(d, &pll, nullptr, &lit_pl) != VK_SUCCESS) {
      return false;
    }

    VkPipelineLayoutCreateInfo plc{};
    plc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plc.setLayoutCount = 1;
    plc.pSetLayouts = &comp_dsl;
    if (vkCreatePipelineLayout(d, &plc, nullptr, &comp_pl) != VK_SUCCESS) {
      return false;
    }

    auto mk_gfx = [&](VkRenderPass rp, VkShaderModule vs, VkShaderModule fs, bool depth_test,
                      VkPipelineLayout layout, VkPipeline* pipe, bool depth_bias_enable) -> bool {
      VkPipelineShaderStageCreateInfo st[2]{};
      st[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      st[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      st[0].module = vs;
      st[0].pName = "main";
      st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      st[1].module = fs;
      st[1].pName = "main";
      VkVertexInputBindingDescription vb{};
      vb.binding = 0;
      vb.stride = sizeof(float) * 8;
      vb.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      VkVertexInputAttributeDescription va[3]{};
      va[0].binding = 0;
      va[0].location = 0;
      va[0].format = VK_FORMAT_R32G32B32_SFLOAT;
      va[0].offset = 0;
      va[1].binding = 0;
      va[1].location = 1;
      va[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      va[1].offset = sizeof(float) * 3;
      va[2].binding = 0;
      va[2].location = 2;
      va[2].format = VK_FORMAT_R32G32_SFLOAT;
      va[2].offset = sizeof(float) * 6;
      VkPipelineVertexInputStateCreateInfo vi{};
      vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      vi.vertexBindingDescriptionCount = 1;
      vi.pVertexBindingDescriptions = &vb;
      vi.vertexAttributeDescriptionCount = 3;
      vi.pVertexAttributeDescriptions = va;
      VkPipelineInputAssemblyStateCreateInfo ia{};
      ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      VkPipelineViewportStateCreateInfo vp{};
      vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      vp.viewportCount = 1;
      vp.scissorCount = 1;
      VkPipelineRasterizationStateCreateInfo rs{};
      rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rs.cullMode = VK_CULL_MODE_BACK_BIT;
      rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
      rs.lineWidth = 1.f;
      rs.depthBiasEnable = depth_bias_enable ? VK_TRUE : VK_FALSE;
      VkPipelineMultisampleStateCreateInfo ms{};
      ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      VkPipelineDepthStencilStateCreateInfo ds{};
      ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      ds.depthTestEnable = depth_test ? VK_TRUE : VK_FALSE;
      ds.depthWriteEnable = depth_test ? VK_TRUE : VK_FALSE;
      ds.depthCompareOp = VK_COMPARE_OP_LESS;
      VkPipelineColorBlendAttachmentState cba{};
      cba.colorWriteMask = 0xf;
      VkPipelineColorBlendStateCreateInfo cb{};
      cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      cb.attachmentCount = 1;
      cb.pAttachments = &cba;
      VkDynamicState dyn3[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS};
      VkDynamicState dyn2[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
      VkPipelineDynamicStateCreateInfo dsc{};
      dsc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      if (depth_bias_enable) {
        dsc.dynamicStateCount = 3;
        dsc.pDynamicStates = dyn3;
      } else {
        dsc.dynamicStateCount = 2;
        dsc.pDynamicStates = dyn2;
      }
      VkGraphicsPipelineCreateInfo gp{};
      gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      gp.stageCount = 2;
      gp.pStages = st;
      gp.pVertexInputState = &vi;
      gp.pInputAssemblyState = &ia;
      gp.pViewportState = &vp;
      gp.pRasterizationState = &rs;
      gp.pMultisampleState = &ms;
      gp.pDepthStencilState = &ds;
      gp.pColorBlendState = &cb;
      gp.pDynamicState = &dsc;
      gp.layout = layout;
      gp.renderPass = rp;
      gp.subpass = 0;
      return vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gp, nullptr, pipe) == VK_SUCCESS;
    };

    auto mk_comp = [&](VkRenderPass rp) -> bool {
      VkPipelineShaderStageCreateInfo st[2]{};
      st[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      st[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
      st[0].module = fsq_vs;
      st[0].pName = "main";
      st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      st[1].module = comp_fs;
      st[1].pName = "main";
      VkPipelineVertexInputStateCreateInfo vi{};
      vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      VkPipelineInputAssemblyStateCreateInfo ia{};
      ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      VkPipelineViewportStateCreateInfo vp{};
      vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      vp.viewportCount = 1;
      vp.scissorCount = 1;
      VkPipelineRasterizationStateCreateInfo rs{};
      rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rs.cullMode = VK_CULL_MODE_NONE;
      rs.lineWidth = 1.f;
      VkPipelineMultisampleStateCreateInfo ms{};
      ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      VkPipelineDepthStencilStateCreateInfo ds{};
      ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      VkPipelineColorBlendAttachmentState cba{};
      cba.colorWriteMask = 0xf;
      VkPipelineColorBlendStateCreateInfo cb{};
      cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      cb.attachmentCount = 1;
      cb.pAttachments = &cba;
      VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
      VkPipelineDynamicStateCreateInfo dsc{};
      dsc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      dsc.dynamicStateCount = 2;
      dsc.pDynamicStates = dyn;
      VkGraphicsPipelineCreateInfo gp{};
      gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      gp.stageCount = 2;
      gp.pStages = st;
      gp.pVertexInputState = &vi;
      gp.pInputAssemblyState = &ia;
      gp.pViewportState = &vp;
      gp.pRasterizationState = &rs;
      gp.pMultisampleState = &ms;
      gp.pDepthStencilState = &ds;
      gp.pColorBlendState = &cb;
      gp.pDynamicState = &dsc;
      gp.layout = comp_pl;
      gp.renderPass = rp;
      gp.subpass = 0;
      return vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gp, nullptr, &comp_pipe) == VK_SUCCESS;
    };

    if (!mk_gfx(shadow_rp, shadow_vs, shadow_fs, true, shadow_pl, &shadow_pipe, true)) {
      return false;
    }
    if (!mk_gfx(hdr_rp, lit_vs, lit_fs, true, lit_pl, &lit_pipe, false)) {
      return false;
    }
    if (!mk_comp(ctx.swapchain_render_pass)) {
      return false;
    }
    return true;
  }

  bool upload_mesh() {
    // P3N3T2: 24 verts (cube), plane 4 verts; indices
    const float verts[] = {
        // +Z
        -0.5f, -0.5f, 0.5f, 0, 0, 1, 0, 0,
        0.5f, -0.5f, 0.5f, 0, 0, 1, 1, 0,
        0.5f, 0.5f, 0.5f, 0, 0, 1, 1, 1,
        -0.5f, 0.5f, 0.5f, 0, 0, 1, 0, 1,
        // -Z
        0.5f, -0.5f, -0.5f, 0, 0, -1, 0, 0,
        -0.5f, -0.5f, -0.5f, 0, 0, -1, 1, 0,
        -0.5f, 0.5f, -0.5f, 0, 0, -1, 1, 1,
        0.5f, 0.5f, -0.5f, 0, 0, -1, 0, 1,
        // +X
        0.5f, -0.5f, 0.5f, 1, 0, 0, 0, 0,
        0.5f, -0.5f, -0.5f, 1, 0, 0, 1, 0,
        0.5f, 0.5f, -0.5f, 1, 0, 0, 1, 1,
        0.5f, 0.5f, 0.5f, 1, 0, 0, 0, 1,
        // -X
        -0.5f, -0.5f, -0.5f, -1, 0, 0, 0, 0,
        -0.5f, -0.5f, 0.5f, -1, 0, 0, 1, 0,
        -0.5f, 0.5f, 0.5f, -1, 0, 0, 1, 1,
        -0.5f, 0.5f, -0.5f, -1, 0, 0, 0, 1,
        // +Y
        -0.5f, 0.5f, 0.5f, 0, 1, 0, 0, 0,
        0.5f, 0.5f, 0.5f, 0, 1, 0, 1, 0,
        0.5f, 0.5f, -0.5f, 0, 1, 0, 1, 1,
        -0.5f, 0.5f, -0.5f, 0, 1, 0, 0, 1,
        // -Y
        -0.5f, -0.5f, -0.5f, 0, -1, 0, 0, 0,
        0.5f, -0.5f, -0.5f, 0, -1, 0, 1, 0,
        0.5f, -0.5f, 0.5f, 0, -1, 0, 1, 1,
        -0.5f, -0.5f, 0.5f, 0, -1, 0, 0, 1,
        // ground plane (large)
        -4.f, 0.f, -4.f, 0, 1, 0, 0, 0,
        4.f, 0.f, -4.f, 0, 1, 0, 4, 0,
        4.f, 0.f, 4.f, 0, 1, 0, 4, 4,
        -4.f, 0.f, 4.f, 0, 1, 0, 0, 4};
    const std::uint16_t idx[] = {
        0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,  8,  9,  10, 10, 11, 8,  12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20, 24, 25, 26, 26, 27, 24};
    VkDevice d = ctx.device;
    VkPhysicalDevice p = ctx.physical;
    VkDeviceSize vsz = sizeof(verts);
    VkDeviceSize isz = sizeof(idx);
    if (!mk_buffer(d, p, vsz, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vb, vb_mem)) {
      return false;
    }
    if (!mk_buffer(d, p, isz, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ib, ib_mem)) {
      return false;
    }
    VkBuffer stg_v{};
    VkDeviceMemory stg_vm{};
    VkBuffer stg_i{};
    VkDeviceMemory stg_im{};
    if (!mk_buffer(d, p, vsz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stg_v, stg_vm)) {
      return false;
    }
    if (!mk_buffer(d, p, isz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stg_i, stg_im)) {
      vkDestroyBuffer(d, stg_v, nullptr);
      vkFreeMemory(d, stg_vm, nullptr);
      return false;
    }
    void* m1{};
    vkMapMemory(d, stg_vm, 0, vsz, 0, &m1);
    std::memcpy(m1, verts, static_cast<size_t>(vsz));
    vkUnmapMemory(d, stg_vm);
    void* m2{};
    vkMapMemory(d, stg_im, 0, isz, 0, &m2);
    std::memcpy(m2, idx, static_cast<size_t>(isz));
    vkUnmapMemory(d, stg_im);

    vkWaitForFences(d, 1, &up_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(d, 1, &up_fence);
    vkResetCommandBuffer(up_cmd, 0);
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(up_cmd, &bi);
    VkBufferCopy c1{};
    c1.size = vsz;
    vkCmdCopyBuffer(up_cmd, stg_v, vb, 1, &c1);
    VkBufferCopy c2{};
    c2.size = isz;
    vkCmdCopyBuffer(up_cmd, stg_i, ib, 1, &c2);
    vkEndCommandBuffer(up_cmd);
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &up_cmd;
    vkQueueSubmit(ctx.graphics_queue, 1, &si, up_fence);
    vkWaitForFences(d, 1, &up_fence, VK_TRUE, UINT64_MAX);
    vkDestroyBuffer(d, stg_v, nullptr);
    vkFreeMemory(d, stg_vm, nullptr);
    vkDestroyBuffer(d, stg_i, nullptr);
    vkFreeMemory(d, stg_im, nullptr);
    idx_count = static_cast<std::uint32_t>(sizeof(idx) / sizeof(idx[0]));
    return true;
  }
};

LitDemoRecorder::LitDemoRecorder() : impl_(std::make_unique<Impl>()) {}
LitDemoRecorder::~LitDemoRecorder() {
  shutdown();
}

bool LitDemoRecorder::is_ready() const {
  return impl_ && impl_->ready;
}

void LitDemoRecorder::shutdown() {
  if (!impl_) {
    return;
  }
  if (impl_->ctx.device == VK_NULL_HANDLE) {
    impl_->ctx = {};
    return;
  }
  vkDeviceWaitIdle(impl_->ctx.device);
  impl_->nukem(impl_->ctx.device);
  impl_->ctx = {};
}

bool LitDemoRecorder::init(const LitDemoInitCtx& ctx) {
  shutdown();
  impl_->ctx = ctx;
  if (!ctx.device || !ctx.physical || !ctx.swapchain_render_pass || ctx.swapchain_extent.width == 0 ||
      ctx.swapchain_extent.height == 0) {
    impl_->ctx = {};
    return false;
  }
  wchar_t buf[MAX_PATH]{};
  if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) {
    impl_->ctx = {};
    return false;
  }
  impl_->shader_dir = std::filesystem::path(buf).parent_path() / "shaders";
  impl_->ext = ctx.swapchain_extent;

  VkDevice d = ctx.device;
  VkPhysicalDevice phys = ctx.physical;

  VkCommandPoolCreateInfo cpci{};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  cpci.queueFamilyIndex = ctx.graphics_queue_family;
  if (vkCreateCommandPool(d, &cpci, nullptr, &impl_->up_pool) != VK_SUCCESS) {
    shutdown();
    return false;
  }
  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.commandPool = impl_->up_pool;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(d, &cbai, &impl_->up_cmd) != VK_SUCCESS) {
    shutdown();
    return false;
  }
  VkFenceCreateInfo fci{};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(d, &fci, nullptr, &impl_->up_fence) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  constexpr std::uint32_t shw = 1024;
  if (!mk_image2d(d, phys, shw, shw, VK_FORMAT_D32_SFLOAT,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_IMAGE_ASPECT_DEPTH_BIT, impl_->shadow)) {
    shutdown();
    return false;
  }

  {
    VkAttachmentDescription ad{};
    ad.format = VK_FORMAT_D32_SFLOAT;
    ad.samples = VK_SAMPLE_COUNT_1_BIT;
    ad.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ad.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ad.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkAttachmentReference ar{};
    ar.attachment = 0;
    ar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription sd{};
    sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sd.pDepthStencilAttachment = &ar;
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &ad;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sd;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    if (vkCreateRenderPass(d, &rpci, nullptr, &impl_->shadow_rp) != VK_SUCCESS) {
      shutdown();
      return false;
    }
    VkFramebufferCreateInfo fbci{};
    fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.renderPass = impl_->shadow_rp;
    fbci.attachmentCount = 1;
    fbci.pAttachments = &impl_->shadow.view;
    fbci.width = shw;
    fbci.height = shw;
    fbci.layers = 1;
    if (vkCreateFramebuffer(d, &fbci, nullptr, &impl_->shadow_fb) != VK_SUCCESS) {
      shutdown();
      return false;
    }
  }

  VkSamplerCreateInfo sci{};
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.magFilter = VK_FILTER_LINEAR;
  sci.minFilter = VK_FILTER_LINEAR;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  if (vkCreateSampler(d, &sci, nullptr, &impl_->shadow_samp) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  if (!impl_->pick_hdr_format(phys)) {
    shutdown();
    return false;
  }
  if (!impl_->build_hdr_pass()) {
    shutdown();
    return false;
  }
  if (!impl_->alloc_hdr_images()) {
    shutdown();
    return false;
  }

  VkDescriptorSetLayoutBinding lb[2]{};
  lb[0].binding = 0;
  lb[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lb[0].descriptorCount = 1;
  lb[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  lb[1].binding = 1;
  lb[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  lb[1].descriptorCount = 1;
  lb[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo dsl{};
  dsl.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dsl.bindingCount = 2;
  dsl.pBindings = lb;
  if (vkCreateDescriptorSetLayout(d, &dsl, nullptr, &impl_->lit_dsl) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  VkDescriptorSetLayoutBinding cb{};
  cb.binding = 0;
  cb.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  cb.descriptorCount = 1;
  cb.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  VkDescriptorSetLayoutCreateInfo dslc{};
  dslc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dslc.bindingCount = 1;
  dslc.pBindings = &cb;
  if (vkCreateDescriptorSetLayout(d, &dslc, nullptr, &impl_->comp_dsl) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  std::array<VkDescriptorPoolSize, 2> ps{};
  ps[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ps[0].descriptorCount = 4;
  ps[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  ps[1].descriptorCount = 8;
  VkDescriptorPoolCreateInfo dpci{};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.maxSets = 4;
  dpci.poolSizeCount = static_cast<std::uint32_t>(ps.size());
  dpci.pPoolSizes = ps.data();
  if (vkCreateDescriptorPool(d, &dpci, nullptr, &impl_->dpool) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  VkDescriptorSetAllocateInfo dsai{};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = impl_->dpool;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &impl_->lit_dsl;
  if (vkAllocateDescriptorSets(d, &dsai, &impl_->lit_set) != VK_SUCCESS) {
    shutdown();
    return false;
  }
  dsai.pSetLayouts = &impl_->comp_dsl;
  if (vkAllocateDescriptorSets(d, &dsai, &impl_->comp_set) != VK_SUCCESS) {
    shutdown();
    return false;
  }

  if (!mk_buffer(d, phys, 256, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, impl_->ubo,
                impl_->ubo_mem)) {
    shutdown();
    return false;
  }
  vkMapMemory(d, impl_->ubo_mem, 0, 256, 0, &impl_->ubo_map);

  if (!impl_->upload_mesh()) {
    shutdown();
    return false;
  }

  impl_->write_lit_desc();
  impl_->write_comp_desc();

  if (!impl_->load_shaders_and_pipelines()) {
    shutdown();
    return false;
  }

  if (ctx.native_window_handle && ctx.instance) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(static_cast<HWND>(ctx.native_window_handle));

    VkDescriptorPoolSize ps[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 32},
                                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256},
                                 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 64},
                                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 64},
                                 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 32},
                                 {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 32},
                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128},
                                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64},
                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 32},
                                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 32},
                                 {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32}};
    VkDescriptorPoolCreateInfo dpci{};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpci.maxSets = 512;
    dpci.poolSizeCount = static_cast<std::uint32_t>(sizeof(ps) / sizeof(ps[0]));
    dpci.pPoolSizes = ps;
    if (vkCreateDescriptorPool(d, &dpci, nullptr, &impl_->imgui_pool) != VK_SUCCESS) {
      shutdown();
      return false;
    }

    ImGui_ImplVulkan_InitInfo vinfo{};
    vinfo.Instance = ctx.instance;
    vinfo.PhysicalDevice = phys;
    vinfo.Device = d;
    vinfo.QueueFamily = ctx.graphics_queue_family;
    vinfo.Queue = ctx.graphics_queue;
    vinfo.DescriptorPool = impl_->imgui_pool;
    vinfo.RenderPass = ctx.swapchain_render_pass;
    vinfo.MinImageCount = 2;
    vinfo.ImageCount = std::max<std::uint32_t>(2u, ctx.swapchain_image_count);
    vinfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    if (!ImGui_ImplVulkan_Init(&vinfo)) {
      shutdown();
      return false;
    }
    if (!ImGui_ImplVulkan_CreateFontsTexture()) {
      ImGui_ImplVulkan_Shutdown();
      vkDestroyDescriptorPool(d, impl_->imgui_pool, nullptr);
      impl_->imgui_pool = VK_NULL_HANDLE;
      ImGui_ImplWin32_Shutdown();
      ImGui::DestroyContext();
      shutdown();
      return false;
    }
    impl_->imgui_inited = true;
  }

  impl_->ready = true;
  return true;
}

void LitDemoRecorder::on_swapchain_resized(VkExtent2D extent, VkFormat swapchain_format,
                                            VkRenderPass swapchain_rp, std::uint32_t image_count) {
  (void)swapchain_format;
  (void)image_count;
  if (!impl_->ready || impl_->ctx.device == VK_NULL_HANDLE) {
    return;
  }
  VkDevice d = impl_->ctx.device;
  vkDeviceWaitIdle(d);
  if (impl_->comp_pipe) {
    vkDestroyPipeline(d, impl_->comp_pipe, nullptr);
    impl_->comp_pipe = VK_NULL_HANDLE;
  }
  for (VkShaderModule* m : {&impl_->comp_fs, &impl_->fsq_vs}) {
    if (*m) {
      vkDestroyShaderModule(d, *m, nullptr);
      *m = VK_NULL_HANDLE;
    }
  }
  if (impl_->hdr_fb) {
    vkDestroyFramebuffer(d, impl_->hdr_fb, nullptr);
    impl_->hdr_fb = VK_NULL_HANDLE;
  }
  if (impl_->hdr_rp) {
    vkDestroyRenderPass(d, impl_->hdr_rp, nullptr);
    impl_->hdr_rp = VK_NULL_HANDLE;
  }
  destroy_gimg(d, impl_->hdr);
  destroy_gimg(d, impl_->hdr_depth);
  impl_->hdr_color_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  impl_->hdr_depth_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (impl_->lit_pipe) {
    vkDestroyPipeline(d, impl_->lit_pipe, nullptr);
    impl_->lit_pipe = VK_NULL_HANDLE;
  }
  for (VkShaderModule* m : {&impl_->lit_fs, &impl_->lit_vs}) {
    if (*m) {
      vkDestroyShaderModule(d, *m, nullptr);
      *m = VK_NULL_HANDLE;
    }
  }
  impl_->ext = extent;
  impl_->ctx.swapchain_extent = extent;
  impl_->ctx.swapchain_render_pass = swapchain_rp;
  if (!impl_->build_hdr_pass() || !impl_->alloc_hdr_images()) {
    impl_->ready = false;
    return;
  }
  impl_->write_lit_desc();
  impl_->write_comp_desc();
  if (!impl_->create_shader_mod(d, read_spv(impl_->shader_dir / "lit_mesh.vert.spv"), &impl_->lit_vs) ||
      !impl_->create_shader_mod(d, read_spv(impl_->shader_dir / "lit_mesh.frag.spv"), &impl_->lit_fs) ||
      !impl_->create_shader_mod(d, read_spv(impl_->shader_dir / "fullscreen.vert.spv"), &impl_->fsq_vs) ||
      !impl_->create_shader_mod(d, read_spv(impl_->shader_dir / "composite.frag.spv"), &impl_->comp_fs)) {
    impl_->ready = false;
    return;
  }
  VkPushConstantRange pcr{};
  pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pcr.offset = 0;
  pcr.size = 64;
  VkPipelineShaderStageCreateInfo st_lit[2]{};
  st_lit[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  st_lit[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  st_lit[0].module = impl_->lit_vs;
  st_lit[0].pName = "main";
  st_lit[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  st_lit[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  st_lit[1].module = impl_->lit_fs;
  st_lit[1].pName = "main";
  VkVertexInputBindingDescription vbb{};
  vbb.binding = 0;
  vbb.stride = sizeof(float) * 8;
  vbb.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  VkVertexInputAttributeDescription va[3]{};
  va[0].binding = 0;
  va[0].location = 0;
  va[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  va[0].offset = 0;
  va[1].binding = 0;
  va[1].location = 1;
  va[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  va[1].offset = sizeof(float) * 3;
  va[2].binding = 0;
  va[2].location = 2;
  va[2].format = VK_FORMAT_R32G32_SFLOAT;
  va[2].offset = sizeof(float) * 6;
  VkPipelineVertexInputStateCreateInfo vi{};
  vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = &vbb;
  vi.vertexAttributeDescriptionCount = 3;
  vi.pVertexAttributeDescriptions = va;
  VkPipelineInputAssemblyStateCreateInfo ia{};
  ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPipelineViewportStateCreateInfo vp{};
  vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vp.viewportCount = 1;
  vp.scissorCount = 1;
  VkPipelineRasterizationStateCreateInfo rs{};
  rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rs.cullMode = VK_CULL_MODE_BACK_BIT;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.f;
  VkPipelineMultisampleStateCreateInfo ms{};
  ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  VkPipelineDepthStencilStateCreateInfo ds{};
  ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = VK_TRUE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS;
  VkPipelineColorBlendAttachmentState cba{};
  cba.colorWriteMask = 0xf;
  VkPipelineColorBlendStateCreateInfo cb{};
  cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  cb.attachmentCount = 1;
  cb.pAttachments = &cba;
  VkDynamicState dyn_lit[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dsc{};
  dsc.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dsc.dynamicStateCount = 2;
  dsc.pDynamicStates = dyn_lit;
  rs.depthBiasEnable = VK_FALSE;
  VkGraphicsPipelineCreateInfo gp{};
  gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gp.stageCount = 2;
  gp.pStages = st_lit;
  gp.pVertexInputState = &vi;
  gp.pInputAssemblyState = &ia;
  gp.pViewportState = &vp;
  gp.pRasterizationState = &rs;
  gp.pMultisampleState = &ms;
  gp.pDepthStencilState = &ds;
  gp.pColorBlendState = &cb;
  gp.pDynamicState = &dsc;
  gp.layout = impl_->lit_pl;
  gp.renderPass = impl_->hdr_rp;
  gp.subpass = 0;
  if (vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gp, nullptr, &impl_->lit_pipe) != VK_SUCCESS) {
    impl_->ready = false;
    return;
  }

  VkPipelineShaderStageCreateInfo stc[2]{};
  stc[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stc[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stc[0].module = impl_->fsq_vs;
  stc[0].pName = "main";
  stc[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stc[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stc[1].module = impl_->comp_fs;
  stc[1].pName = "main";
  VkPipelineVertexInputStateCreateInfo vi0{};
  vi0.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  VkPipelineInputAssemblyStateCreateInfo ia0{};
  ia0.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia0.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkPipelineViewportStateCreateInfo vp0{};
  vp0.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vp0.viewportCount = 1;
  vp0.scissorCount = 1;
  VkPipelineRasterizationStateCreateInfo rs0{};
  rs0.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rs0.cullMode = VK_CULL_MODE_NONE;
  rs0.lineWidth = 1.f;
  VkPipelineMultisampleStateCreateInfo ms0{};
  ms0.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  ms0.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  VkPipelineDepthStencilStateCreateInfo ds0{};
  ds0.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  VkPipelineColorBlendAttachmentState cba0{};
  cba0.colorWriteMask = 0xf;
  VkPipelineColorBlendStateCreateInfo cb0{};
  cb0.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  cb0.attachmentCount = 1;
  cb0.pAttachments = &cba0;
  VkDynamicState dyn0[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dsc0{};
  dsc0.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dsc0.dynamicStateCount = 2;
  dsc0.pDynamicStates = dyn0;
  VkGraphicsPipelineCreateInfo gpc{};
  gpc.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gpc.stageCount = 2;
  gpc.pStages = stc;
  gpc.pVertexInputState = &vi0;
  gpc.pInputAssemblyState = &ia0;
  gpc.pViewportState = &vp0;
  gpc.pRasterizationState = &rs0;
  gpc.pMultisampleState = &ms0;
  gpc.pDepthStencilState = &ds0;
  gpc.pColorBlendState = &cb0;
  gpc.pDynamicState = &dsc0;
  gpc.layout = impl_->comp_pl;
  gpc.renderPass = swapchain_rp;
  gpc.subpass = 0;
  if (vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &gpc, nullptr, &impl_->comp_pipe) != VK_SUCCESS) {
    impl_->ready = false;
  }

  if (impl_->imgui_inited) {
    ImGui_ImplVulkan_SetMinImageCount(std::max<std::uint32_t>(2u, image_count));
  }
}

bool LitDemoRecorder::record(VkCommandBuffer cmd, VkFramebuffer swapchain_fb, VkExtent2D extent,
                             const float clear_rgba[4], const LitDemoFrameParams* frame) {
  if (!impl_->ready) {
    return false;
  }
  Impl& I = *impl_;
  VkDevice d = I.ctx.device;

  const float demo_t = frame ? frame->demo_time_seconds : 0.f;
  float eye[3]{};
  float at[3] = {0, 0.4f, 0};
  float up[3] = {0, 1, 0};
  if (frame && frame->use_mouse_camera) {
    const float cx = frame->look_at[0];
    const float cy = frame->look_at[1];
    const float cz = frame->look_at[2];
    const float dist = frame->orbit_distance;
    const float cyaw = std::cos(frame->yaw_rad);
    const float syaw = std::sin(frame->yaw_rad);
    const float cp = std::cos(frame->pitch_rad);
    const float sp = std::sin(frame->pitch_rad);
    const float dx = cyaw * cp * dist;
    const float dy = sp * dist;
    const float dz = syaw * cp * dist;
    eye[0] = cx + dx;
    eye[1] = cy + dy;
    eye[2] = cz + dz;
    at[0] = cx;
    at[1] = cy;
    at[2] = cz;
  } else {
    eye[0] = std::cos(demo_t * 0.35f) * 4.f;
    eye[1] = 2.6f;
    eye[2] = std::sin(demo_t * 0.35f) * 4.f;
    at[0] = 0;
    at[1] = 0.4f;
    at[2] = 0;
  }

  // UBO: view, proj, light_vp, light_dir, cam_pos (std140 approx)
  float view[16], proj[16], light_v[16], light_p[16], light_vp_bias[16];
  mat_look(eye, at, up, view);
  const float aspect =
      static_cast<float>(extent.width) / std::max<std::uint32_t>(1u, extent.height);
  mat_persp(55.f, aspect, 0.1f, 50.f, proj);
  float ldir[3] = {0.45f, -0.85f, 0.25f};
  float ll = std::sqrt(ldir[0] * ldir[0] + ldir[1] * ldir[1] + ldir[2] * ldir[2]);
  ldir[0] /= ll;
  ldir[1] /= ll;
  ldir[2] /= ll;
  float leye[3] = {-ldir[0] * 8.f, -ldir[1] * 8.f + 2.f, -ldir[2] * 8.f};
  float lat[3] = {0, 0, 0};
  mat_look(leye, lat, up, light_v);
  mat_ortho(-3.f, 3.f, -3.f, 3.f, 0.1f, 25.f, light_p);
  float light_vp_raw[16];
  mat_mul(light_p, light_v, light_vp_raw);
  float bias[16] = {0.5f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 1.f, 0, 0.5f, 0.5f, 0, 1.f};
  mat_mul(bias, light_vp_raw, light_vp_bias);

  std::uint8_t* ub = static_cast<std::uint8_t*>(I.ubo_map);
  std::memcpy(ub + 0, view, 64);
  std::memcpy(ub + 64, proj, 64);
  std::memcpy(ub + 128, light_vp_bias, 64);
  float ld4[4] = {ldir[0], ldir[1], ldir[2], 0.f};
  std::memcpy(ub + 192, ld4, 16);
  float cp4[4] = {eye[0], eye[1], eye[2], 0.f};
  std::memcpy(ub + 208, cp4, 16);

  {
    const VkImageLayout sold = I.shadow_layout;
    vulkan::cmd_image_barrier(cmd, I.shadow.img, VK_IMAGE_ASPECT_DEPTH_BIT, sold,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                sold == VK_IMAGE_LAYOUT_UNDEFINED ? 0u : VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                sold == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                  : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
    I.shadow_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkClearValue sc{};
  sc.depthStencil = {1.f, 0};
  VkRenderPassBeginInfo srp{};
  srp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  srp.renderPass = I.shadow_rp;
  srp.framebuffer = I.shadow_fb;
  srp.renderArea.offset = {0, 0};
  srp.renderArea.extent = {1024, 1024};
  srp.clearValueCount = 1;
  srp.pClearValues = &sc;
  vkCmdBeginRenderPass(cmd, &srp, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, I.shadow_pipe);
  VkViewport svp{};
  svp.width = 1024;
  svp.height = 1024;
  svp.minDepth = 0;
  svp.maxDepth = 1;
  VkRect2D ssc{};
  ssc.extent = {1024, 1024};
  vkCmdSetViewport(cmd, 0, 1, &svp);
  vkCmdSetScissor(cmd, 0, 1, &ssc);
  vkCmdSetDepthBias(cmd, 1.25f, 0.f, 1.75f);
  const VkDeviceSize z0 = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &I.vb, &z0);
  vkCmdBindIndexBuffer(cmd, I.ib, 0, VK_INDEX_TYPE_UINT16);
  float cube_m[16];
  mat_id(cube_m);
  {
    float cr = std::cos(demo_t);
    float sr = std::sin(demo_t);
    cube_m[0] = cr;
    cube_m[2] = sr;
    cube_m[8] = -sr;
    cube_m[10] = cr;
    cube_m[13] = 0.6f;
  }
  if (frame) {
    cube_m[12] += frame->lit_cube_translate[0];
    cube_m[13] += frame->lit_cube_translate[1];
    cube_m[14] += frame->lit_cube_translate[2];
  }
  float smvp_c[16];
  mat_mul(light_vp_raw, cube_m, smvp_c);
  vkCmdPushConstants(cmd, I.shadow_pl, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, smvp_c);
  vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
  float idm[16];
  mat_id(idm);
  float smvp_p[16];
  mat_mul(light_vp_raw, idm, smvp_p);
  vkCmdPushConstants(cmd, I.shadow_pl, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, smvp_p);
  vkCmdDrawIndexed(cmd, 6, 1, 36, 0, 0);
  vkCmdEndRenderPass(cmd);

  vulkan::cmd_image_barrier(cmd, I.shadow.img, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
              VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  I.shadow_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

  {
    const VkImageLayout cold = I.hdr_color_layout;
    vulkan::cmd_image_barrier(cmd, I.hdr.img, VK_IMAGE_ASPECT_COLOR_BIT, cold, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                cold == VK_IMAGE_LAYOUT_UNDEFINED ? 0u
                                                  : (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT),
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                cold == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                  : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    I.hdr_color_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
  {
    const VkImageLayout d0 = I.hdr_depth_layout;
    vulkan::cmd_image_barrier(cmd, I.hdr_depth.img, VK_IMAGE_ASPECT_DEPTH_BIT, d0,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                d0 == VK_IMAGE_LAYOUT_UNDEFINED ? 0u : VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                d0 == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                                                : VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
    I.hdr_depth_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkClearValue clears[2]{};
  clears[0].color = {{0.02f, 0.03f, 0.06f, 1.f}};
  clears[1].depthStencil = {1.f, 0};
  VkRenderPassBeginInfo hrp{};
  hrp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  hrp.renderPass = I.hdr_rp;
  hrp.framebuffer = I.hdr_fb;
  hrp.renderArea.offset = {0, 0};
  hrp.renderArea.extent = extent;
  hrp.clearValueCount = 2;
  hrp.pClearValues = clears;
  vkCmdBeginRenderPass(cmd, &hrp, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, I.lit_pipe);
  VkViewport hvp{};
  hvp.width = static_cast<float>(extent.width);
  hvp.height = static_cast<float>(extent.height);
  hvp.minDepth = 0;
  hvp.maxDepth = 1;
  VkRect2D hsc{};
  hsc.extent = extent;
  vkCmdSetViewport(cmd, 0, 1, &hvp);
  vkCmdSetScissor(cmd, 0, 1, &hsc);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, I.lit_pl, 0, 1, &I.lit_set, 0, nullptr);
  vkCmdBindVertexBuffers(cmd, 0, 1, &I.vb, &z0);
  vkCmdBindIndexBuffer(cmd, I.ib, 0, VK_INDEX_TYPE_UINT16);
  vkCmdPushConstants(cmd, I.lit_pl, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, cube_m);
  vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
  float pm[16];
  mat_id(pm);
  vkCmdPushConstants(cmd, I.lit_pl, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, pm);
  vkCmdDrawIndexed(cmd, 6, 1, 36, 0, 0);
  vkCmdEndRenderPass(cmd);

  vulkan::cmd_image_barrier(cmd, I.hdr.img, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
              VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  I.hdr_color_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkRenderPassBeginInfo prp{};
  prp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  prp.renderPass = I.ctx.swapchain_render_pass;
  prp.framebuffer = swapchain_fb;
  prp.renderArea.offset = {0, 0};
  prp.renderArea.extent = extent;
  VkClearValue pc{};
  pc.color.float32[0] = clear_rgba[0];
  pc.color.float32[1] = clear_rgba[1];
  pc.color.float32[2] = clear_rgba[2];
  pc.color.float32[3] = clear_rgba[3];
  prp.clearValueCount = 1;
  prp.pClearValues = &pc;
  vkCmdBeginRenderPass(cmd, &prp, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, I.comp_pipe);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, I.comp_pl, 0, 1, &I.comp_set, 0, nullptr);
  vkCmdSetViewport(cmd, 0, 1, &hvp);
  vkCmdSetScissor(cmd, 0, 1, &hsc);
  vkCmdDraw(cmd, 3, 1, 0, 0);

  if (frame && frame->imgui_draw_data && I.imgui_inited) {
    ImGui_ImplVulkan_RenderDrawData(static_cast<ImDrawData*>(frame->imgui_draw_data), cmd);
  }

  vkCmdEndRenderPass(cmd);

  (void)d;
  return true;
}

}  // namespace weavebound::rhi
