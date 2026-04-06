// Win64 + Vulkan：instance / surface / device / swapchain（M1 最小閉環）
// NOMINMAX 必須早於 <vulkan/vulkan.h>（其於 Win32 會帶入 windows.h），否則 std::min/max 與巨集衝突。
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdio>
#include <unordered_set>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "weavebound/platform/window.hpp"
#include "weavebound/rhi/device.hpp"
#include "weavebound/render_graph/compiled.hpp"
#include "weavebound/rhi/lit_demo_vulkan.hpp"
#include "weavebound/rhi/vulkan/rg_executor_vulkan.hpp"

namespace weavebound::rhi {

namespace {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   VkDebugUtilsMessageTypeFlagsEXT,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                   void*) {
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT && data && data->pMessage) {
    std::fprintf(stderr, "[Vulkan] %s\n", data->pMessage);
  }
  return VK_FALSE;
}

static bool has_validation_layer() {
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, nullptr);
  std::vector<VkLayerProperties> layers(count);
  vkEnumerateInstanceLayerProperties(&count, layers.data());
  for (const auto& l : layers) {
    if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
      return true;
    }
  }
  return false;
}

static std::vector<char> read_spv_file(const std::filesystem::path& path) {
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

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& caps, int req_w, int req_h) {
  if (caps.currentExtent.width != UINT32_MAX && caps.currentExtent.height != UINT32_MAX) {
    return caps.currentExtent;
  }
  VkExtent2D e{static_cast<uint32_t>(std::max(1, req_w)), static_cast<uint32_t>(std::max(1, req_h))};
  e.width = std::clamp(e.width, caps.minImageExtent.width, caps.maxImageExtent.width);
  e.height = std::clamp(e.height, caps.minImageExtent.height, caps.maxImageExtent.height);
  return e;
}

static std::uint32_t find_memory_type(VkPhysicalDevice phys, std::uint32_t type_bits,
                                      VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties mp{};
  vkGetPhysicalDeviceMemoryProperties(phys, &mp);
  for (std::uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
    if ((type_bits & (1u << i)) != 0 &&
        (mp.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  return UINT32_MAX;
}

struct GpuBufferEntry {
  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceMemory memory{VK_NULL_HANDLE};
  VkDeviceSize size{0};
};

struct GpuImageEntry {
  VkImage image{VK_NULL_HANDLE};
  VkDeviceMemory memory{VK_NULL_HANDLE};
  VkImageView view{VK_NULL_HANDLE};
  VkExtent2D extent{};
  VkFormat format{VK_FORMAT_UNDEFINED};
};

class VulkanDevice final : public IDevice {
 public:
  VulkanDevice() = default;
  ~VulkanDevice() override { shutdown(); }

  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice& operator=(const VulkanDevice&) = delete;

  bool init(const DeviceDesc& desc);

  Backend backend() const override { return Backend::Vulkan; }

  bool is_valid() const override { return valid_; }

  bool clear_present_rgba(float r, float g, float b, float a,
                          const rg::CompiledRenderGraph* render_graph,
                          const LitDemoFrameParams* lit) override;

  bool create_buffer(const BufferDesc& desc, BufferHandle& out) override;

  bool upload_buffer(const BufferHandle& buffer, const void* data, std::size_t size,
                     std::size_t offset = 0) override;

  bool dispatch_compute(std::uint32_t group_x, std::uint32_t group_y,
                        std::uint32_t group_z) override;

  bool create_image(const ImageDesc& desc, ImageHandle& out) override;
  void destroy_image(const ImageHandle& image) override;

  VkImage rg_transient_vk_image(rg::ResourceId id) const {
    const auto it = rg_transient_images_.find(id);
    return it == rg_transient_images_.end() ? VK_NULL_HANDLE : it->second.image;
  }

 private:
  static constexpr int kFramesInFlight = 2;

  void shutdown();
  void destroy_present_pipeline();

  bool create_instance(const DeviceDesc& desc);
  bool create_surface(const DeviceDesc& desc);
  bool pick_physical_device();
  bool create_logical_device();
  bool create_swapchain(const DeviceDesc& desc, VkSwapchainKHR old_swapchain = VK_NULL_HANDLE);
  bool create_present_pipeline();
  void destroy_triangle_pipeline();
  void try_load_triangle_pipeline();
  void destroy_all_gpu_buffers();
  bool create_vulkan_buffer_entry(const BufferDesc& desc, GpuBufferEntry& out);
  VkBuffer vk_buffer(BufferHandle h) const;
  bool ensure_mesh_geometry();
  bool try_build_compute();
  void destroy_compute();

  void load_debug_utils_cmd_procs();

  void destroy_rg_transient_images();
  void ensure_rg_transient_images(const rg::CompiledRenderGraph& crg);
  bool create_swapchain_framebuffers();
  void destroy_swapchain_framebuffers_and_views();
  bool try_rebuild_swapchain_if_needed();
  bool create_gpu_image_2d(std::uint32_t width_px, std::uint32_t height_px, VkFormat vk_format,
                           VkImageUsageFlags vk_usage, VkImageAspectFlags aspect_mask, bool create_view,
                           GpuImageEntry& out);
  void destroy_gpu_image(GpuImageEntry& e);
  bool valid_{false};
  platform::IWindow* window_{nullptr};
  DeviceDesc cached_desc_{};
  bool debug_utils_labels_{false};
  PFN_vkCmdBeginDebugUtilsLabelEXT pfn_cmd_begin_debug_utils_label_{nullptr};
  PFN_vkCmdEndDebugUtilsLabelEXT pfn_cmd_end_debug_utils_label_{nullptr};
  VkInstance instance_{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkPhysicalDevice physical_{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};
  VkQueue graphics_queue_{VK_NULL_HANDLE};
  VkQueue present_queue_{VK_NULL_HANDLE};
  uint32_t graphics_family_{UINT32_MAX};
  uint32_t present_family_{UINT32_MAX};

  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  VkFormat swapchain_format_{VK_FORMAT_UNDEFINED};
  VkExtent2D swapchain_extent_{};
  std::vector<VkImage> swapchain_images_;
  std::vector<VkImageView> swapchain_views_;

  VkRenderPass render_pass_{VK_NULL_HANDLE};
  std::vector<VkFramebuffer> framebuffers_;
  VkCommandPool cmd_pool_{VK_NULL_HANDLE};
  VkCommandBuffer cmd_bufs_[kFramesInFlight]{};
  VkSemaphore sem_image_available_[kFramesInFlight]{};
  VkSemaphore sem_render_done_[kFramesInFlight]{};
  VkFence fences_[kFramesInFlight]{};
  int frame_index_{0};

  std::unordered_map<std::uint64_t, GpuBufferEntry> buffers_;
  std::uint64_t next_buffer_id_{1};
  std::unordered_map<std::uint64_t, GpuImageEntry> gpu_images_;
  std::uint64_t next_image_id_{1};
  std::unordered_map<rg::ResourceId, GpuImageEntry> rg_transient_images_;
  VkBuffer draw_vb_{VK_NULL_HANDLE};
  VkBuffer draw_ib_{VK_NULL_HANDLE};
  std::uint32_t draw_index_count_{0};

  VkPipeline compute_pipeline_{VK_NULL_HANDLE};
  VkPipelineLayout compute_layout_{VK_NULL_HANDLE};
  VkDescriptorPool compute_desc_pool_{VK_NULL_HANDLE};
  VkDescriptorSetLayout compute_set_layout_{VK_NULL_HANDLE};
  VkDescriptorSet compute_set_{VK_NULL_HANDLE};
  VkShaderModule compute_mod_{VK_NULL_HANDLE};
  BufferHandle compute_ssbo_id_{};
  VkFence compute_fence_{VK_NULL_HANDLE};
  VkCommandBuffer compute_cmd_{VK_NULL_HANDLE};
  bool compute_ready_{false};

  VkCommandPool upload_pool_{VK_NULL_HANDLE};
  VkCommandBuffer upload_cmd_{VK_NULL_HANDLE};
  VkFence upload_fence_{VK_NULL_HANDLE};

  VkPipeline gfx_pipeline_{VK_NULL_HANDLE};
  VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
  VkShaderModule vert_mod_{VK_NULL_HANDLE};
  VkShaderModule frag_mod_{VK_NULL_HANDLE};
  bool triangle_ready_{false};

  std::unique_ptr<LitDemoRecorder> lit_demo_{};
  bool init_lit_demo();
};

bool VulkanDevice::create_instance(const DeviceDesc& desc) {
  VkApplicationInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  ai.pApplicationName = "WeaveBound";
  ai.applicationVersion = VK_MAKE_VERSION(0, 2, 3);
  ai.apiVersion = VK_API_VERSION_1_1;

  std::vector<const char*> extensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
  const bool want_debug = desc.enable_debug_layers && has_validation_layer();
  if (want_debug) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  const char* layer_names[] = {"VK_LAYER_KHRONOS_validation"};
  VkInstanceCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &ai;
  ici.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ici.ppEnabledExtensionNames = extensions.data();
  if (want_debug) {
    ici.enabledLayerCount = 1;
    ici.ppEnabledLayerNames = layer_names;
  }

  if (vkCreateInstance(&ici, nullptr, &instance_) != VK_SUCCESS) {
    return false;
  }

  if (want_debug) {
    auto fn_create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (fn_create) {
      VkDebugUtilsMessengerCreateInfoEXT dmci{};
      dmci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      dmci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      dmci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      dmci.pfnUserCallback = debug_callback;
      fn_create(instance_, &dmci, nullptr, &debug_messenger_);
    }
  }
  return true;
}

bool VulkanDevice::create_surface(const DeviceDesc& desc) {
  auto fn = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(instance_, "vkCreateWin32SurfaceKHR"));
  if (!fn) {
    return false;
  }
  VkWin32SurfaceCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  HINSTANCE hi = static_cast<HINSTANCE>(desc.surface_target->native_display_handle());
  if (!hi) {
    hi = GetModuleHandleW(nullptr);
  }
  sci.hinstance = hi;
  sci.hwnd = static_cast<HWND>(desc.surface_target->native_window_handle());
  return fn(instance_, &sci, nullptr, &surface_) == VK_SUCCESS;
}

bool VulkanDevice::pick_physical_device() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance_, &count, nullptr);
  if (count == 0) {
    return false;
  }
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(instance_, &count, devices.data());

  auto score = [](VkPhysicalDevice pd) {
    VkPhysicalDeviceProperties p{};
    vkGetPhysicalDeviceProperties(pd, &p);
    int s = 1;
    if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      s += 1000;
    } else if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      s += 100;
    }
    return s;
  };

  VkPhysicalDevice best = VK_NULL_HANDLE;
  int best_score = -1;
  for (VkPhysicalDevice pd : devices) {
    const int s = score(pd);
    if (s > best_score) {
      best_score = s;
      best = pd;
    }
  }
  if (best == VK_NULL_HANDLE) {
    return false;
  }

  uint32_t qf = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(best, &qf, nullptr);
  std::vector<VkQueueFamilyProperties> props(qf);
  vkGetPhysicalDeviceQueueFamilyProperties(best, &qf, props.data());

  graphics_family_ = UINT32_MAX;
  present_family_ = UINT32_MAX;
  bool combined = false;
  for (uint32_t i = 0; i < qf; ++i) {
    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(best, i, surface_, &present_support);
    const bool graphics = (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    if (graphics && present_support) {
      graphics_family_ = i;
      present_family_ = i;
      combined = true;
      break;
    }
  }
  if (!combined) {
    for (uint32_t i = 0; i < qf; ++i) {
      if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
        graphics_family_ = i;
        break;
      }
    }
    for (uint32_t i = 0; i < qf; ++i) {
      VkBool32 present_support = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(best, i, surface_, &present_support);
      if (present_support) {
        present_family_ = i;
        break;
      }
    }
  }

  if (graphics_family_ == UINT32_MAX || present_family_ == UINT32_MAX) {
    return false;
  }

  physical_ = best;
  return true;
}

bool VulkanDevice::create_logical_device() {
  const float qp = 1.f;
  std::vector<VkDeviceQueueCreateInfo> qcis;
  const uint32_t families[] = {graphics_family_, present_family_};
  if (graphics_family_ == present_family_) {
    VkDeviceQueueCreateInfo q{};
    q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q.queueFamilyIndex = graphics_family_;
    q.queueCount = 1;
    q.pQueuePriorities = &qp;
    qcis.push_back(q);
  } else {
    for (uint32_t fam : families) {
      VkDeviceQueueCreateInfo q{};
      q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      q.queueFamilyIndex = fam;
      q.queueCount = 1;
      q.pQueuePriorities = &qp;
      qcis.push_back(q);
    }
  }

  const char* exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dci{};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.queueCreateInfoCount = static_cast<uint32_t>(qcis.size());
  dci.pQueueCreateInfos = qcis.data();
  dci.enabledExtensionCount = 1;
  dci.ppEnabledExtensionNames = exts;

  if (vkCreateDevice(physical_, &dci, nullptr, &device_) != VK_SUCCESS) {
    return false;
  }

  vkGetDeviceQueue(device_, graphics_family_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, present_family_, 0, &present_queue_);
  return true;
}

bool VulkanDevice::create_swapchain(const DeviceDesc& desc, VkSwapchainKHR old_swapchain) {
  VkSurfaceCapabilitiesKHR caps{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_, surface_, &caps) != VK_SUCCESS) {
    return false;
  }

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_, surface_, &format_count, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(format_count);
  if (format_count > 0) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_, surface_, &format_count, formats.data());
  }

  VkSurfaceFormatKHR chosen = formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                                                : formats[0];
  for (const auto& f : formats) {
    if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      chosen = f;
      break;
    }
  }

  uint32_t mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_, surface_, &mode_count, nullptr);
  std::vector<VkPresentModeKHR> modes(mode_count);
  if (mode_count > 0) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_, surface_, &mode_count, modes.data());
  }
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  for (VkPresentModeKHR m : modes) {
    if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
      present_mode = m;
      break;
    }
  }

  const VkExtent2D extent =
      choose_extent(caps, desc.surface_target->width(), desc.surface_target->height());

  uint32_t image_count = caps.minImageCount + 1;
  if (image_count < 3) {
    image_count = 3;
  }
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) {
    image_count = caps.maxImageCount;
  }
  if (image_count < caps.minImageCount) {
    image_count = caps.minImageCount;
  }

  VkSwapchainCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = surface_;
  sci.minImageCount = image_count;
  sci.imageFormat = chosen.format;
  sci.imageColorSpace = chosen.colorSpace;
  sci.imageExtent = extent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  const uint32_t qfams[] = {graphics_family_, present_family_};
  if (graphics_family_ != present_family_) {
    sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = qfams;
  } else {
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  sci.preTransform = caps.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = present_mode;
  sci.clipped = VK_TRUE;
  sci.oldSwapchain = old_swapchain;

  if (vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_) != VK_SUCCESS) {
    return false;
  }

  swapchain_format_ = chosen.format;
  swapchain_extent_ = extent;

  uint32_t img_count = 0;
  vkGetSwapchainImagesKHR(device_, swapchain_, &img_count, nullptr);
  swapchain_images_.resize(img_count);
  if (vkGetSwapchainImagesKHR(device_, swapchain_, &img_count, swapchain_images_.data()) != VK_SUCCESS) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
    swapchain_images_.clear();
    return false;
  }

  std::vector<VkImageView> views;
  views.reserve(img_count);
  for (uint32_t i = 0; i < img_count; ++i) {
    VkImageViewCreateInfo iv{};
    iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv.image = swapchain_images_[i];
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = swapchain_format_;
    iv.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    iv.subresourceRange.baseMipLevel = 0;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.baseArrayLayer = 0;
    iv.subresourceRange.layerCount = 1;
    VkImageView v{};
    if (vkCreateImageView(device_, &iv, nullptr, &v) != VK_SUCCESS) {
      for (VkImageView x : views) {
        vkDestroyImageView(device_, x, nullptr);
      }
      vkDestroySwapchainKHR(device_, swapchain_, nullptr);
      swapchain_ = VK_NULL_HANDLE;
      swapchain_images_.clear();
      return false;
    }
    views.push_back(v);
  }
  swapchain_views_ = std::move(views);

  return true;
}

bool VulkanDevice::create_swapchain_framebuffers() {
  for (VkFramebuffer fb : framebuffers_) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device_, fb, nullptr);
    }
  }
  framebuffers_.clear();
  const size_t n = swapchain_views_.size();
  if (n == 0 || render_pass_ == VK_NULL_HANDLE) {
    return false;
  }
  framebuffers_.resize(n);
  for (size_t i = 0; i < n; ++i) {
    VkFramebufferCreateInfo fbci{};
    fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.renderPass = render_pass_;
    fbci.attachmentCount = 1;
    fbci.pAttachments = &swapchain_views_[i];
    fbci.width = swapchain_extent_.width;
    fbci.height = swapchain_extent_.height;
    fbci.layers = 1;
    if (vkCreateFramebuffer(device_, &fbci, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
      for (size_t j = 0; j < i; ++j) {
        vkDestroyFramebuffer(device_, framebuffers_[j], nullptr);
        framebuffers_[j] = VK_NULL_HANDLE;
      }
      framebuffers_.clear();
      return false;
    }
  }
  return true;
}

bool VulkanDevice::create_present_pipeline() {
  VkAttachmentDescription ad{};
  ad.format = swapchain_format_;
  ad.samples = VK_SAMPLE_COUNT_1_BIT;
  ad.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  ad.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  ad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ad.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference ar{};
  ar.attachment = 0;
  ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription sd{};
  sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  sd.colorAttachmentCount = 1;
  sd.pColorAttachments = &ar;

  VkSubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.srcAccessMask = 0;
  dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo rpci{};
  rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpci.attachmentCount = 1;
  rpci.pAttachments = &ad;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &sd;
  rpci.dependencyCount = 1;
  rpci.pDependencies = &dep;
  if (vkCreateRenderPass(device_, &rpci, nullptr, &render_pass_) != VK_SUCCESS) {
    return false;
  }

  if (!create_swapchain_framebuffers()) {
    destroy_present_pipeline();
    return false;
  }

  VkCommandPoolCreateInfo cpci{};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = graphics_family_;
  if (vkCreateCommandPool(device_, &cpci, nullptr, &cmd_pool_) != VK_SUCCESS) {
    destroy_present_pipeline();
    return false;
  }

  VkCommandBufferAllocateInfo cbai{};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.commandPool = cmd_pool_;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = kFramesInFlight;
  if (vkAllocateCommandBuffers(device_, &cbai, cmd_bufs_) != VK_SUCCESS) {
    destroy_present_pipeline();
    return false;
  }

  VkSemaphoreCreateInfo sci{};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fci{};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (int i = 0; i < kFramesInFlight; ++i) {
    if (vkCreateSemaphore(device_, &sci, nullptr, &sem_image_available_[i]) != VK_SUCCESS) {
      destroy_present_pipeline();
      return false;
    }
    if (vkCreateSemaphore(device_, &sci, nullptr, &sem_render_done_[i]) != VK_SUCCESS) {
      destroy_present_pipeline();
      return false;
    }
    if (vkCreateFence(device_, &fci, nullptr, &fences_[i]) != VK_SUCCESS) {
      destroy_present_pipeline();
      return false;
    }
  }

  VkCommandPoolCreateInfo upci{};
  upci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  upci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  upci.queueFamilyIndex = graphics_family_;
  if (vkCreateCommandPool(device_, &upci, nullptr, &upload_pool_) != VK_SUCCESS) {
    destroy_present_pipeline();
    return false;
  }
  VkCommandBufferAllocateInfo ucb{};
  ucb.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ucb.commandPool = upload_pool_;
  ucb.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ucb.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(device_, &ucb, &upload_cmd_) != VK_SUCCESS) {
    destroy_present_pipeline();
    return false;
  }
  VkFenceCreateInfo uf{};
  uf.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(device_, &uf, nullptr, &upload_fence_) != VK_SUCCESS) {
    destroy_present_pipeline();
    return false;
  }

  try_load_triangle_pipeline();
  try_build_compute();
  if (cached_desc_.enable_lit_demo) {
    init_lit_demo();
  }
  return true;
}

bool VulkanDevice::init_lit_demo() {
  lit_demo_ = std::make_unique<LitDemoRecorder>();
  LitDemoInitCtx lc{};
  lc.instance = instance_;
  lc.device = device_;
  lc.physical = physical_;
  lc.graphics_queue = graphics_queue_;
  lc.graphics_queue_family = graphics_family_;
  lc.command_pool = cmd_pool_;
  lc.swapchain_render_pass = render_pass_;
  lc.swapchain_format = swapchain_format_;
  lc.swapchain_extent = swapchain_extent_;
  lc.swapchain_image_count = static_cast<std::uint32_t>(swapchain_images_.size());
  lc.native_window_handle = window_ ? window_->native_window_handle() : nullptr;
  if (!lit_demo_->init(lc)) {
    lit_demo_.reset();
    return false;
  }
  return true;
}

void VulkanDevice::destroy_triangle_pipeline() {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  if (gfx_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, gfx_pipeline_, nullptr);
    gfx_pipeline_ = VK_NULL_HANDLE;
  }
  if (pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;
  }
  if (vert_mod_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_, vert_mod_, nullptr);
    vert_mod_ = VK_NULL_HANDLE;
  }
  if (frag_mod_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_, frag_mod_, nullptr);
    frag_mod_ = VK_NULL_HANDLE;
  }
  triangle_ready_ = false;
}

void VulkanDevice::try_load_triangle_pipeline() {
  destroy_triangle_pipeline();
  if (!ensure_mesh_geometry()) {
    return;
  }
  wchar_t buf[MAX_PATH]{};
  if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) {
    return;
  }
  const std::filesystem::path exe_dir = std::filesystem::path(buf).parent_path();
  const std::filesystem::path shader_dir = exe_dir / "shaders";
  const std::vector<char> vc = read_spv_file(shader_dir / "triangle.vert.spv");
  const std::vector<char> fc = read_spv_file(shader_dir / "triangle.frag.spv");
  if (vc.empty() || fc.empty()) {
    return;
  }

  VkShaderModuleCreateInfo vsm{};
  vsm.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vsm.codeSize = vc.size();
  vsm.pCode = reinterpret_cast<const uint32_t*>(vc.data());
  if (vkCreateShaderModule(device_, &vsm, nullptr, &vert_mod_) != VK_SUCCESS) {
    vert_mod_ = VK_NULL_HANDLE;
    return;
  }

  VkShaderModuleCreateInfo fsm{};
  fsm.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fsm.codeSize = fc.size();
  fsm.pCode = reinterpret_cast<const uint32_t*>(fc.data());
  if (vkCreateShaderModule(device_, &fsm, nullptr, &frag_mod_) != VK_SUCCESS) {
    vkDestroyShaderModule(device_, vert_mod_, nullptr);
    vert_mod_ = frag_mod_ = VK_NULL_HANDLE;
    return;
  }

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vert_mod_;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = frag_mod_;
  stages[1].pName = "main";

  VkVertexInputBindingDescription vbind{};
  vbind.binding = 0;
  vbind.stride = sizeof(float) * 6;
  vbind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription vattr[2]{};
  vattr[0].binding = 0;
  vattr[0].location = 0;
  vattr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vattr[0].offset = 0;
  vattr[1].binding = 0;
  vattr[1].location = 1;
  vattr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vattr[1].offset = sizeof(float) * 3;

  VkPipelineVertexInputStateCreateInfo vi{};
  vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = &vbind;
  vi.vertexAttributeDescriptionCount = 2;
  vi.pVertexAttributeDescriptions = vattr;

  VkPipelineInputAssemblyStateCreateInfo ia{};
  ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineViewportStateCreateInfo vps{};
  vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vps.viewportCount = 1;
  vps.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rs{};
  rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rs.depthClampEnable = VK_FALSE;
  rs.rasterizerDiscardEnable = VK_FALSE;
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.lineWidth = 1.f;
  rs.cullMode = VK_CULL_MODE_NONE;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo ms{};
  ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState cba{};
  cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                       VK_COLOR_COMPONENT_A_BIT;
  cba.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo cb{};
  cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  cb.logicOpEnable = VK_FALSE;
  cb.attachmentCount = 1;
  cb.pAttachments = &cba;

  VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo ds{};
  ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  ds.dynamicStateCount = 2;
  ds.pDynamicStates = dyn;

  VkPipelineLayoutCreateInfo pl{};
  pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  if (vkCreatePipelineLayout(device_, &pl, nullptr, &pipeline_layout_) != VK_SUCCESS) {
    vkDestroyShaderModule(device_, frag_mod_, nullptr);
    vkDestroyShaderModule(device_, vert_mod_, nullptr);
    frag_mod_ = vert_mod_ = VK_NULL_HANDLE;
    return;
  }

  VkGraphicsPipelineCreateInfo gp{};
  gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  gp.stageCount = 2;
  gp.pStages = stages;
  gp.pVertexInputState = &vi;
  gp.pInputAssemblyState = &ia;
  gp.pViewportState = &vps;
  gp.pRasterizationState = &rs;
  gp.pMultisampleState = &ms;
  gp.pColorBlendState = &cb;
  gp.pDynamicState = &ds;
  gp.layout = pipeline_layout_;
  gp.renderPass = render_pass_;
  gp.subpass = 0;
  gp.basePipelineHandle = VK_NULL_HANDLE;
  gp.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &gp, nullptr, &gfx_pipeline_) != VK_SUCCESS) {
    vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;
    vkDestroyShaderModule(device_, frag_mod_, nullptr);
    vkDestroyShaderModule(device_, vert_mod_, nullptr);
    frag_mod_ = vert_mod_ = VK_NULL_HANDLE;
    return;
  }

  triangle_ready_ = true;
}

void VulkanDevice::destroy_present_pipeline() {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  (void)vkDeviceWaitIdle(device_);
  lit_demo_.reset();
  destroy_rg_transient_images();
  destroy_compute();
  destroy_triangle_pipeline();

  if (upload_fence_ != VK_NULL_HANDLE) {
    vkDestroyFence(device_, upload_fence_, nullptr);
    upload_fence_ = VK_NULL_HANDLE;
  }
  if (upload_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, upload_pool_, nullptr);
    upload_pool_ = VK_NULL_HANDLE;
    upload_cmd_ = VK_NULL_HANDLE;
  }
  for (int i = 0; i < kFramesInFlight; ++i) {
    if (fences_[i] != VK_NULL_HANDLE) {
      vkDestroyFence(device_, fences_[i], nullptr);
      fences_[i] = VK_NULL_HANDLE;
    }
    if (sem_render_done_[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, sem_render_done_[i], nullptr);
      sem_render_done_[i] = VK_NULL_HANDLE;
    }
    if (sem_image_available_[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, sem_image_available_[i], nullptr);
      sem_image_available_[i] = VK_NULL_HANDLE;
    }
    cmd_bufs_[i] = VK_NULL_HANDLE;
  }
  if (cmd_pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device_, cmd_pool_, nullptr);
    cmd_pool_ = VK_NULL_HANDLE;
  }
  for (VkFramebuffer fb : framebuffers_) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device_, fb, nullptr);
    }
  }
  framebuffers_.clear();
  if (render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }
}

void VulkanDevice::load_debug_utils_cmd_procs() {
  if (!debug_utils_labels_ || device_ == VK_NULL_HANDLE) {
    return;
  }
  pfn_cmd_begin_debug_utils_label_ = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
      vkGetDeviceProcAddr(device_, "vkCmdBeginDebugUtilsLabelEXT"));
  pfn_cmd_end_debug_utils_label_ = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
      vkGetDeviceProcAddr(device_, "vkCmdEndDebugUtilsLabelEXT"));
  if (!pfn_cmd_begin_debug_utils_label_ || !pfn_cmd_end_debug_utils_label_) {
    pfn_cmd_begin_debug_utils_label_ = nullptr;
    pfn_cmd_end_debug_utils_label_ = nullptr;
  }
}

static VkImage vulkan_resolve_rg_image(void* user, rg::ResourceId id);

bool VulkanDevice::clear_present_rgba(float r, float g, float b, float a,
                                      const rg::CompiledRenderGraph* render_graph, const LitDemoFrameParams* lit) {
  if (!valid_ || cmd_bufs_[0] == VK_NULL_HANDLE || swapchain_ == VK_NULL_HANDLE) {
    return false;
  }

  if (!try_rebuild_swapchain_if_needed()) {
    return false;
  }

  const int fi = frame_index_;
  vkWaitForFences(device_, 1, &fences_[fi], VK_TRUE, UINT64_MAX);

  uint32_t image_index = 0;
  constexpr std::uint64_t kAcquireTimeoutNs = 1'500'000'000ULL;
  VkResult ar = vkAcquireNextImageKHR(device_, swapchain_, kAcquireTimeoutNs, sem_image_available_[fi],
                                      VK_NULL_HANDLE, &image_index);
  if (ar == VK_TIMEOUT || ar == VK_NOT_READY) {
    return false;
  }
  if (ar == VK_ERROR_OUT_OF_DATE_KHR) {
    if (!try_rebuild_swapchain_if_needed()) {
      return false;
    }
    ar = vkAcquireNextImageKHR(device_, swapchain_, kAcquireTimeoutNs, sem_image_available_[fi], VK_NULL_HANDLE,
                               &image_index);
    if (ar == VK_TIMEOUT || ar == VK_NOT_READY) {
      return false;
    }
  }
  if (ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR) {
    return false;
  }

  vkResetFences(device_, 1, &fences_[fi]);

  VkCommandBuffer cmd = cmd_bufs_[fi];
  vkResetCommandBuffer(cmd, 0);

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) {
    return false;
  }

  if (render_graph != nullptr && render_graph->ok) {
    ensure_rg_transient_images(*render_graph);
    record_render_graph_vulkan(cmd, *render_graph, pfn_cmd_begin_debug_utils_label_, pfn_cmd_end_debug_utils_label_,
                               vulkan_resolve_rg_image, static_cast<void*>(this));
  }

  const bool lit_ok = lit_demo_ && lit_demo_->is_ready();
  if (lit_ok) {
    const float cc[4] = {r, g, b, a};
    lit_demo_->record(cmd, framebuffers_[image_index], swapchain_extent_, cc, lit);
  } else {
    VkClearValue clear{};
    clear.color.float32[0] = r;
    clear.color.float32[1] = g;
    clear.color.float32[2] = b;
    clear.color.float32[3] = a;

    VkRenderPassBeginInfo rpbi{};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = render_pass_;
    rpbi.framebuffer = framebuffers_[image_index];
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = swapchain_extent_;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clear;

    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    if (triangle_ready_ && gfx_pipeline_ != VK_NULL_HANDLE && draw_vb_ != VK_NULL_HANDLE &&
        draw_ib_ != VK_NULL_HANDLE) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx_pipeline_);
      VkViewport vp{};
      vp.x = 0.f;
      vp.y = 0.f;
      vp.width = static_cast<float>(swapchain_extent_.width);
      vp.height = static_cast<float>(swapchain_extent_.height);
      vp.minDepth = 0.f;
      vp.maxDepth = 1.f;
      vkCmdSetViewport(cmd, 0, 1, &vp);
      VkRect2D sc{};
      sc.offset = {0, 0};
      sc.extent = swapchain_extent_;
      vkCmdSetScissor(cmd, 0, 1, &sc);
      const VkDeviceSize off = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &draw_vb_, &off);
      vkCmdBindIndexBuffer(cmd, draw_ib_, 0, VK_INDEX_TYPE_UINT16);
      vkCmdDrawIndexed(cmd, draw_index_count_, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(cmd);
  }

  if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
    return false;
  }

  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = &sem_image_available_[fi];
  si.pWaitDstStageMask = &wait_stage;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = &sem_render_done_[fi];

  if (vkQueueSubmit(graphics_queue_, 1, &si, fences_[fi]) != VK_SUCCESS) {
    return false;
  }

  VkPresentInfoKHR pi{};
  pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  pi.waitSemaphoreCount = 1;
  pi.pWaitSemaphores = &sem_render_done_[fi];
  pi.swapchainCount = 1;
  pi.pSwapchains = &swapchain_;
  pi.pImageIndices = &image_index;

  const VkResult pr = vkQueuePresentKHR(present_queue_, &pi);
  frame_index_ = (frame_index_ + 1) % kFramesInFlight;
  if (pr == VK_ERROR_OUT_OF_DATE_KHR) {
    return true;
  }
  if (pr != VK_SUCCESS && pr != VK_SUBOPTIMAL_KHR) {
    return false;
  }
  return true;
}

static VkFormat pixel_format_to_vk(PixelFormat p) {
  switch (p) {
    case PixelFormat::R8G8B8A8_UNORM:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case PixelFormat::B8G8R8A8_UNORM:
      return VK_FORMAT_B8G8R8A8_UNORM;
    case PixelFormat::D32_FLOAT:
      return VK_FORMAT_D32_SFLOAT;
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

void VulkanDevice::destroy_gpu_image(GpuImageEntry& e) {
  if (device_ == VK_NULL_HANDLE) {
    e = {};
    return;
  }
  if (e.view != VK_NULL_HANDLE) {
    vkDestroyImageView(device_, e.view, nullptr);
    e.view = VK_NULL_HANDLE;
  }
  if (e.image != VK_NULL_HANDLE) {
    vkDestroyImage(device_, e.image, nullptr);
    e.image = VK_NULL_HANDLE;
  }
  if (e.memory != VK_NULL_HANDLE) {
    vkFreeMemory(device_, e.memory, nullptr);
    e.memory = VK_NULL_HANDLE;
  }
  e.extent = {};
  e.format = VK_FORMAT_UNDEFINED;
}

void VulkanDevice::destroy_rg_transient_images() {
  if (device_ == VK_NULL_HANDLE) {
    rg_transient_images_.clear();
    return;
  }
  for (auto& kv : rg_transient_images_) {
    destroy_gpu_image(kv.second);
  }
  rg_transient_images_.clear();
}

bool VulkanDevice::try_rebuild_swapchain_if_needed() {
  if (!window_ || device_ == VK_NULL_HANDLE || swapchain_ == VK_NULL_HANDLE) {
    return true;
  }
  if (render_pass_ == VK_NULL_HANDLE) {
    return true;
  }
  const int w = window_->width();
  const int h = window_->height();
  if (w <= 0 || h <= 0) {
    return false;
  }
  const uint32_t nw = static_cast<uint32_t>(w);
  const uint32_t nh = static_cast<uint32_t>(h);
  if (nw == swapchain_extent_.width && nh == swapchain_extent_.height) {
    return true;
  }
  vkDeviceWaitIdle(device_);
  destroy_rg_transient_images();
  const VkSwapchainKHR old_sc = swapchain_;
  for (VkFramebuffer fb : framebuffers_) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device_, fb, nullptr);
    }
  }
  framebuffers_.clear();
  for (VkImageView v : swapchain_views_) {
    if (v != VK_NULL_HANDLE) {
      vkDestroyImageView(device_, v, nullptr);
    }
  }
  swapchain_views_.clear();
  swapchain_images_.clear();
  swapchain_ = VK_NULL_HANDLE;
  if (!create_swapchain(cached_desc_, old_sc)) {
    return false;
  }
  if (!create_swapchain_framebuffers()) {
    return false;
  }
  destroy_triangle_pipeline();
  try_load_triangle_pipeline();
  if (lit_demo_) {
    lit_demo_->on_swapchain_resized(swapchain_extent_, swapchain_format_, render_pass_,
                                    static_cast<std::uint32_t>(swapchain_images_.size()));
  }
  return true;
}

void VulkanDevice::ensure_rg_transient_images(const rg::CompiledRenderGraph& crg) {
  if (device_ == VK_NULL_HANDLE || swapchain_extent_.width == 0 || swapchain_extent_.height == 0) {
    return;
  }
  std::unordered_set<rg::ResourceId> want;
  for (const auto& r : crg.resources) {
    if (r.transient) {
      want.insert(r.id);
    }
  }
  for (auto it = rg_transient_images_.begin(); it != rg_transient_images_.end();) {
    if (want.count(it->first) == 0) {
      destroy_gpu_image(it->second);
      it = rg_transient_images_.erase(it);
    } else {
      ++it;
    }
  }
  for (rg::ResourceId id : want) {
    auto it = rg_transient_images_.find(id);
    const bool need_new =
        it == rg_transient_images_.end() || it->second.extent.width != swapchain_extent_.width ||
        it->second.extent.height != swapchain_extent_.height || it->second.format != swapchain_format_;
    if (!need_new) {
      continue;
    }
    if (it != rg_transient_images_.end()) {
      destroy_gpu_image(it->second);
      rg_transient_images_.erase(it);
    }
    GpuImageEntry img{};
    const VkImageUsageFlags u = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (!create_gpu_image_2d(swapchain_extent_.width, swapchain_extent_.height, swapchain_format_, u,
                             VK_IMAGE_ASPECT_COLOR_BIT, true, img)) {
      continue;
    }
    rg_transient_images_[id] = img;
  }
}

bool VulkanDevice::create_gpu_image_2d(std::uint32_t width_px, std::uint32_t height_px, VkFormat vk_format,
                                       VkImageUsageFlags vk_usage, VkImageAspectFlags aspect_mask,
                                       bool create_view, GpuImageEntry& out) {
  if (device_ == VK_NULL_HANDLE || width_px == 0 || height_px == 0 || vk_format == VK_FORMAT_UNDEFINED) {
    return false;
  }
  VkImageCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = vk_format;
  ici.extent.width = width_px;
  ici.extent.height = height_px;
  ici.extent.depth = 1;
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = vk_usage;
  ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateImage(device_, &ici, nullptr, &out.image) != VK_SUCCESS) {
    return false;
  }
  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device_, out.image, &req);
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = find_memory_type(physical_, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mai.memoryTypeIndex == UINT32_MAX) {
    vkDestroyImage(device_, out.image, nullptr);
    out.image = VK_NULL_HANDLE;
    return false;
  }
  if (vkAllocateMemory(device_, &mai, nullptr, &out.memory) != VK_SUCCESS) {
    vkDestroyImage(device_, out.image, nullptr);
    out.image = VK_NULL_HANDLE;
    return false;
  }
  if (vkBindImageMemory(device_, out.image, out.memory, 0) != VK_SUCCESS) {
    vkFreeMemory(device_, out.memory, nullptr);
    vkDestroyImage(device_, out.image, nullptr);
    out.memory = VK_NULL_HANDLE;
    out.image = VK_NULL_HANDLE;
    return false;
  }
  out.extent.width = width_px;
  out.extent.height = height_px;
  out.format = vk_format;
  if (create_view) {
    VkImageViewCreateInfo iv{};
    iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv.image = out.image;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = vk_format;
    iv.subresourceRange.aspectMask = aspect_mask;
    iv.subresourceRange.baseMipLevel = 0;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.baseArrayLayer = 0;
    iv.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &iv, nullptr, &out.view) != VK_SUCCESS) {
      destroy_gpu_image(out);
      return false;
    }
  }
  return true;
}

bool VulkanDevice::create_image(const ImageDesc& desc, ImageHandle& out) {
  if (device_ == VK_NULL_HANDLE || desc.width_px == 0 || desc.height_px == 0) {
    return false;
  }
  const VkFormat fmt = pixel_format_to_vk(desc.format);
  if (fmt == VK_FORMAT_UNDEFINED) {
    return false;
  }
  VkImageUsageFlags u = 0;
  const std::uint32_t mask = as_u32(desc.usage);
  if (mask & as_u32(ImageUsageFlags::Sampled)) {
    u |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  if (mask & as_u32(ImageUsageFlags::ColorAttachment)) {
    u |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }
  if (mask & as_u32(ImageUsageFlags::DepthStencil)) {
    u |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }
  if (mask & as_u32(ImageUsageFlags::TransferDst)) {
    u |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }
  if (mask & as_u32(ImageUsageFlags::TransferSrc)) {
    u |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
  if (u == 0) {
    return false;
  }
  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  if (mask & as_u32(ImageUsageFlags::DepthStencil)) {
    aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  const bool need_view = (mask & as_u32(ImageUsageFlags::Sampled)) != 0 ||
                         (mask & as_u32(ImageUsageFlags::ColorAttachment)) != 0 ||
                         (mask & as_u32(ImageUsageFlags::DepthStencil)) != 0;
  GpuImageEntry e{};
  if (!create_gpu_image_2d(desc.width_px, desc.height_px, fmt, u, aspect, need_view, e)) {
    return false;
  }
  const std::uint64_t id = next_image_id_++;
  gpu_images_[id] = e;
  out.id = id;
  return true;
}

void VulkanDevice::destroy_image(const ImageHandle& image) {
  if (device_ == VK_NULL_HANDLE || image.id == 0) {
    return;
  }
  auto it = gpu_images_.find(image.id);
  if (it == gpu_images_.end()) {
    return;
  }
  destroy_gpu_image(it->second);
  gpu_images_.erase(it);
}

static VkImage vulkan_resolve_rg_image(void* user, rg::ResourceId id) {
  return static_cast<VulkanDevice*>(user)->rg_transient_vk_image(id);
}

void VulkanDevice::destroy_all_gpu_buffers() {
  if (device_ == VK_NULL_HANDLE) {
    buffers_.clear();
    gpu_images_.clear();
    draw_vb_ = VK_NULL_HANDLE;
    draw_ib_ = VK_NULL_HANDLE;
    draw_index_count_ = 0;
    return;
  }
  for (auto& kv : gpu_images_) {
    destroy_gpu_image(kv.second);
  }
  gpu_images_.clear();
  for (auto& kv : buffers_) {
    GpuBufferEntry& e = kv.second;
    if (e.buffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(device_, e.buffer, nullptr);
      e.buffer = VK_NULL_HANDLE;
    }
    if (e.memory != VK_NULL_HANDLE) {
      vkFreeMemory(device_, e.memory, nullptr);
      e.memory = VK_NULL_HANDLE;
    }
  }
  buffers_.clear();
  draw_vb_ = VK_NULL_HANDLE;
  draw_ib_ = VK_NULL_HANDLE;
  draw_index_count_ = 0;
}

bool VulkanDevice::create_vulkan_buffer_entry(const BufferDesc& desc, GpuBufferEntry& out) {
  VkBufferUsageFlags u = 0;
  const std::uint32_t mask = as_u32(desc.usage);
  if (mask & as_u32(BufferUsage::Vertex)) {
    u |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  if (mask & as_u32(BufferUsage::Index)) {
    u |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if (mask & as_u32(BufferUsage::Uniform)) {
    u |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  if (mask & as_u32(BufferUsage::Storage)) {
    u |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }
  if (mask & as_u32(BufferUsage::TransferSrc)) {
    u |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (mask & as_u32(BufferUsage::TransferDst)) {
    u |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (!desc.host_visible) {
    u |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  VkBufferCreateInfo bci{};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = desc.size_bytes;
  bci.usage = u;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_, &bci, nullptr, &out.buffer) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(device_, out.buffer, &req);
  VkMemoryAllocateInfo mai{};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = req.size;
  const VkMemoryPropertyFlags props =
      desc.host_visible ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  mai.memoryTypeIndex = find_memory_type(physical_, req.memoryTypeBits, props);
  if (mai.memoryTypeIndex == UINT32_MAX) {
    vkDestroyBuffer(device_, out.buffer, nullptr);
    out.buffer = VK_NULL_HANDLE;
    return false;
  }

  if (vkAllocateMemory(device_, &mai, nullptr, &out.memory) != VK_SUCCESS) {
    vkDestroyBuffer(device_, out.buffer, nullptr);
    out.buffer = VK_NULL_HANDLE;
    return false;
  }
  if (vkBindBufferMemory(device_, out.buffer, out.memory, 0) != VK_SUCCESS) {
    vkFreeMemory(device_, out.memory, nullptr);
    vkDestroyBuffer(device_, out.buffer, nullptr);
    out.memory = VK_NULL_HANDLE;
    out.buffer = VK_NULL_HANDLE;
    return false;
  }
  out.size = desc.size_bytes;
  return true;
}

bool VulkanDevice::create_buffer(const BufferDesc& desc, BufferHandle& out) {
  if (device_ == VK_NULL_HANDLE || desc.size_bytes == 0 || as_u32(desc.usage) == 0) {
    return false;
  }
  GpuBufferEntry e{};
  if (!create_vulkan_buffer_entry(desc, e)) {
    return false;
  }
  const std::uint64_t id = next_buffer_id_++;
  buffers_[id] = e;
  out.id = id;
  return true;
}

VkBuffer VulkanDevice::vk_buffer(BufferHandle h) const {
  const auto it = buffers_.find(h.id);
  if (it == buffers_.end()) {
    return VK_NULL_HANDLE;
  }
  return it->second.buffer;
}

bool VulkanDevice::upload_buffer(const BufferHandle& buffer, const void* data, std::size_t size, std::size_t offset) {
  if (device_ == VK_NULL_HANDLE || upload_cmd_ == VK_NULL_HANDLE || data == nullptr || size == 0) {
    return false;
  }
  const auto it = buffers_.find(buffer.id);
  if (it == buffers_.end()) {
    return false;
  }
  GpuBufferEntry& dst = it->second;
  if (offset + size > static_cast<std::size_t>(dst.size)) {
    return false;
  }

  void* mapped = nullptr;
  if (vkMapMemory(device_, dst.memory, offset, size, 0, &mapped) == VK_SUCCESS) {
    std::memcpy(mapped, data, size);
    vkUnmapMemory(device_, dst.memory);
    return true;
  }

  GpuBufferEntry stg{};
  BufferDesc st_desc{};
  st_desc.size_bytes = size;
  st_desc.host_visible = true;
  st_desc.usage = BufferUsage::TransferSrc;
  if (!create_vulkan_buffer_entry(st_desc, stg)) {
    return false;
  }
  void* sp = nullptr;
  if (vkMapMemory(device_, stg.memory, 0, size, 0, &sp) != VK_SUCCESS) {
    vkDestroyBuffer(device_, stg.buffer, nullptr);
    vkFreeMemory(device_, stg.memory, nullptr);
    return false;
  }
  std::memcpy(sp, data, size);
  vkUnmapMemory(device_, stg.memory);

  vkWaitForFences(device_, 1, &upload_fence_, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &upload_fence_);
  vkResetCommandBuffer(upload_cmd_, 0);

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(upload_cmd_, &bi) != VK_SUCCESS) {
    vkDestroyBuffer(device_, stg.buffer, nullptr);
    vkFreeMemory(device_, stg.memory, nullptr);
    return false;
  }
  VkBufferCopy region{};
  region.srcOffset = 0;
  region.dstOffset = offset;
  region.size = size;
  vkCmdCopyBuffer(upload_cmd_, stg.buffer, dst.buffer, 1, &region);
  vkEndCommandBuffer(upload_cmd_);

  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &upload_cmd_;
  if (vkQueueSubmit(graphics_queue_, 1, &si, upload_fence_) != VK_SUCCESS) {
    vkDestroyBuffer(device_, stg.buffer, nullptr);
    vkFreeMemory(device_, stg.memory, nullptr);
    return false;
  }
  vkWaitForFences(device_, 1, &upload_fence_, VK_TRUE, UINT64_MAX);
  vkDestroyBuffer(device_, stg.buffer, nullptr);
  vkFreeMemory(device_, stg.memory, nullptr);
  return true;
}

bool VulkanDevice::ensure_mesh_geometry() {
  if (draw_vb_ != VK_NULL_HANDLE && draw_ib_ != VK_NULL_HANDLE) {
    return true;
  }
  const float vd[] = {-1.f, -1.f, 0.f, 1.f,  0.2f, 0.2f, 1.f, -1.f, 0.f, 0.2f, 1.f,
                      0.2f, 0.f,  1.f, 0.f,  0.2f, 0.5f, 1.f};
  const std::uint16_t idx[] = {0, 1, 2};
  BufferDesc vbd{};
  vbd.size_bytes = sizeof(vd);
  vbd.host_visible = false;
  vbd.usage = BufferUsage::Vertex | BufferUsage::TransferDst;
  BufferHandle vbh{};
  if (!create_buffer(vbd, vbh)) {
    return false;
  }
  if (!upload_buffer(vbh, vd, sizeof(vd))) {
    return false;
  }
  BufferDesc ibd{};
  ibd.size_bytes = sizeof(idx);
  ibd.host_visible = false;
  ibd.usage = BufferUsage::Index | BufferUsage::TransferDst;
  BufferHandle ibh{};
  if (!create_buffer(ibd, ibh)) {
    return false;
  }
  if (!upload_buffer(ibh, idx, sizeof(idx))) {
    return false;
  }
  draw_vb_ = vk_buffer(vbh);
  draw_ib_ = vk_buffer(ibh);
  draw_index_count_ = 3;
  return draw_vb_ != VK_NULL_HANDLE && draw_ib_ != VK_NULL_HANDLE;
}

void VulkanDevice::destroy_compute() {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  if (compute_fence_ != VK_NULL_HANDLE) {
    vkWaitForFences(device_, 1, &compute_fence_, VK_TRUE, UINT64_MAX);
  }
  if (compute_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, compute_pipeline_, nullptr);
    compute_pipeline_ = VK_NULL_HANDLE;
  }
  if (compute_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, compute_layout_, nullptr);
    compute_layout_ = VK_NULL_HANDLE;
  }
  if (compute_desc_pool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_, compute_desc_pool_, nullptr);
    compute_desc_pool_ = VK_NULL_HANDLE;
  }
  if (compute_set_layout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_, compute_set_layout_, nullptr);
    compute_set_layout_ = VK_NULL_HANDLE;
  }
  if (compute_mod_ != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device_, compute_mod_, nullptr);
    compute_mod_ = VK_NULL_HANDLE;
  }
  if (compute_ssbo_id_.id != 0) {
    const auto it = buffers_.find(compute_ssbo_id_.id);
    if (it != buffers_.end()) {
      if (it->second.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, it->second.buffer, nullptr);
      }
      if (it->second.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_, it->second.memory, nullptr);
      }
      buffers_.erase(it);
    }
    compute_ssbo_id_ = BufferHandle{};
  }
  if (compute_fence_ != VK_NULL_HANDLE) {
    vkDestroyFence(device_, compute_fence_, nullptr);
    compute_fence_ = VK_NULL_HANDLE;
  }
  compute_set_ = VK_NULL_HANDLE;
  compute_ready_ = false;
}

bool VulkanDevice::try_build_compute() {
  destroy_compute();
  wchar_t buf[MAX_PATH]{};
  if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0) {
    return false;
  }
  const std::filesystem::path exe_dir = std::filesystem::path(buf).parent_path();
  const std::vector<char> code = read_spv_file(exe_dir / "shaders" / "trivial.comp.spv");
  if (code.empty()) {
    return false;
  }

  BufferDesc bd{};
  bd.size_bytes = sizeof(std::uint32_t);
  bd.host_visible = false;
  bd.usage = BufferUsage::Storage | BufferUsage::TransferDst;
  if (!create_buffer(bd, compute_ssbo_id_)) {
    return false;
  }
  const std::uint32_t zero = 0;
  if (!upload_buffer(compute_ssbo_id_, &zero, sizeof(zero))) {
    destroy_compute();
    return false;
  }

  VkDescriptorSetLayoutBinding sb{};
  sb.binding = 0;
  sb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  sb.descriptorCount = 1;
  sb.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutCreateInfo slci{};
  slci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  slci.bindingCount = 1;
  slci.pBindings = &sb;
  if (vkCreateDescriptorSetLayout(device_, &slci, nullptr, &compute_set_layout_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  VkPipelineLayoutCreateInfo plci{};
  plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plci.setLayoutCount = 1;
  plci.pSetLayouts = &compute_set_layout_;
  if (vkCreatePipelineLayout(device_, &plci, nullptr, &compute_layout_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  VkShaderModuleCreateInfo sm{};
  sm.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  sm.codeSize = code.size();
  sm.pCode = reinterpret_cast<const std::uint32_t*>(code.data());
  if (vkCreateShaderModule(device_, &sm, nullptr, &compute_mod_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  VkPipelineShaderStageCreateInfo stage{};
  stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stage.module = compute_mod_;
  stage.pName = "main";

  VkComputePipelineCreateInfo cpci{};
  cpci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  cpci.stage = stage;
  cpci.layout = compute_layout_;

  if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &cpci, nullptr, &compute_pipeline_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  VkDescriptorPoolSize ps{};
  ps.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  ps.descriptorCount = 1;
  VkDescriptorPoolCreateInfo dpci{};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.maxSets = 1;
  dpci.poolSizeCount = 1;
  dpci.pPoolSizes = &ps;
  if (vkCreateDescriptorPool(device_, &dpci, nullptr, &compute_desc_pool_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  VkDescriptorSetAllocateInfo dsai{};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = compute_desc_pool_;
  dsai.descriptorSetCount = 1;
  dsai.pSetLayouts = &compute_set_layout_;
  if (vkAllocateDescriptorSets(device_, &dsai, &compute_set_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  const GpuBufferEntry* ssbo = nullptr;
  {
    const auto it = buffers_.find(compute_ssbo_id_.id);
    if (it == buffers_.end()) {
      destroy_compute();
      return false;
    }
    ssbo = &it->second;
  }

  VkDescriptorBufferInfo dbi{};
  dbi.buffer = ssbo->buffer;
  dbi.offset = 0;
  dbi.range = VK_WHOLE_SIZE;

  VkWriteDescriptorSet w{};
  w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  w.dstSet = compute_set_;
  w.dstBinding = 0;
  w.descriptorCount = 1;
  w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  w.pBufferInfo = &dbi;
  vkUpdateDescriptorSets(device_, 1, &w, 0, nullptr);

  VkFenceCreateInfo fci{};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(device_, &fci, nullptr, &compute_fence_) != VK_SUCCESS) {
    destroy_compute();
    return false;
  }

  compute_ready_ = true;
  return true;
}

bool VulkanDevice::dispatch_compute(std::uint32_t group_x, std::uint32_t group_y, std::uint32_t group_z) {
  if (device_ == VK_NULL_HANDLE || !compute_ready_ || group_x == 0 || group_y == 0 || group_z == 0) {
    return false;
  }
  vkWaitForFences(device_, 1, &compute_fence_, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &compute_fence_);
  vkResetCommandBuffer(upload_cmd_, 0);

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(upload_cmd_, &bi) != VK_SUCCESS) {
    return false;
  }
  vkCmdBindPipeline(upload_cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);
  vkCmdBindDescriptorSets(upload_cmd_, VK_PIPELINE_BIND_POINT_COMPUTE, compute_layout_, 0, 1, &compute_set_, 0,
                          nullptr);
  vkCmdDispatch(upload_cmd_, group_x, group_y, group_z);
  vkEndCommandBuffer(upload_cmd_);

  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &upload_cmd_;
  if (vkQueueSubmit(graphics_queue_, 1, &si, compute_fence_) != VK_SUCCESS) {
    return false;
  }
  vkWaitForFences(device_, 1, &compute_fence_, VK_TRUE, UINT64_MAX);
  return true;
}

bool VulkanDevice::init(const DeviceDesc& desc) {
  cached_desc_ = desc;
  window_ = desc.surface_target;
  debug_utils_labels_ = desc.enable_debug_layers && has_validation_layer();
  if (!create_instance(desc)) {
    shutdown();
    return false;
  }
  if (!create_surface(desc)) {
    shutdown();
    return false;
  }
  if (!pick_physical_device()) {
    shutdown();
    return false;
  }
  if (!create_logical_device()) {
    shutdown();
    return false;
  }
  load_debug_utils_cmd_procs();
  if (!create_swapchain(desc)) {
    shutdown();
    return false;
  }
  if (!create_present_pipeline()) {
    shutdown();
    return false;
  }
  valid_ = true;
  return true;
}

void VulkanDevice::shutdown() {
  if (device_ != VK_NULL_HANDLE) {
    destroy_present_pipeline();
    destroy_all_gpu_buffers();
    for (VkImageView v : swapchain_views_) {
      if (v != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, v, nullptr);
      }
    }
    swapchain_views_.clear();
    swapchain_images_.clear();
    if (swapchain_ != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(device_, swapchain_, nullptr);
      swapchain_ = VK_NULL_HANDLE;
    }
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }

  if (instance_ != VK_NULL_HANDLE) {
    if (surface_ != VK_NULL_HANDLE) {
      vkDestroySurfaceKHR(instance_, surface_, nullptr);
      surface_ = VK_NULL_HANDLE;
    }
    if (debug_messenger_ != VK_NULL_HANDLE) {
      auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
      if (fn) {
        fn(instance_, debug_messenger_, nullptr);
      }
      debug_messenger_ = VK_NULL_HANDLE;
    }
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }

  graphics_queue_ = VK_NULL_HANDLE;
  present_queue_ = VK_NULL_HANDLE;
  physical_ = VK_NULL_HANDLE;
  valid_ = false;
}

}  // namespace

std::unique_ptr<IDevice> try_create_vulkan_device(const DeviceDesc& desc) {
  if (desc.backend != Backend::Vulkan) {
    return nullptr;
  }
  if (!desc.surface_target) {
    return nullptr;
  }
  if (!desc.surface_target->native_window_handle()) {
    return nullptr;
  }

  auto dev = std::make_unique<VulkanDevice>();
  if (!dev->init(desc)) {
    return nullptr;
  }
  return dev;
}

}  // namespace weavebound::rhi
