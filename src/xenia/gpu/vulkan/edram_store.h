/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_VULKAN_EDRAM_STORE_H_
#define XENIA_GPU_VULKAN_EDRAM_STORE_H_

#include <memory>

#include "third_party/glslang-spirv/SpvBuilder.h"
#include "xenia/gpu/xenos.h"
#include "xenia/ui/vulkan/fenced_pools.h"
#include "xenia/ui/vulkan/vulkan.h"
#include "xenia/ui/vulkan/vulkan_device.h"

namespace xe {
namespace gpu {
namespace vulkan {

// Stores the raw contents of the EDRAM in a buffer implemented as a 1280x2048
// 32-bit image, basically manages the guest framebuffers.
//
// There is a large comment in render_cache.h describing the way EDRAM is
// structured and accessed by games.
//
// Thanks to the fact that the EDRAM is not directly accessible by the CPU or
// shaders, we don't have to emulate whatever swizzling there may be in EDRAM.
// Instead, we assume that render targets are stored in EDRAM linearly.
//
// A 1280x2048 image is used to make the contents of EDRAM easier to debug, to
// improve data locality and to allow for a special path for 1280x render
// targets with 16-aligned tile offset that uses copying rather than a compute
// shader (this is currently not implemented and not very important).
// In the 1280x2048 image, 16 tiles are laid out every 16-pixel row, and can be
// indexed as ((tile & 15) * 80, (tile >> 4) * 16).
//
// A small API note - when edram_pitch_px is used, it must be pre-multiplied by
// 2 if using 4x MSAA.
class EDRAMStore {
 public:
  EDRAMStore(ui::vulkan::VulkanDevice* device);
  ~EDRAMStore();

  VkResult Initialize();
  void Shutdown();

  // Whether the format is 64 bits per pixel on the guest.
  static inline bool IsColorFormat64bpp(ColorRenderTargetFormat format) {
    return format == ColorRenderTargetFormat::k_16_16_16_16 ||
           format == ColorRenderTargetFormat::k_16_16_16_16_FLOAT ||
           format == ColorRenderTargetFormat::k_32_32_FLOAT;
  }

  // Prior to storing, the render target must be in the following state:
  // StageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  // AccessMask & VK_ACCESS_SHADER_READ_BIT
  // Layout VK_IMAGE_LAYOUT_GENERAL
  // It must be created with usage & VK_IMAGE_USAGE_STORAGE_BIT for color,
  // VK_IMAGE_USAGE_SAMPLED_BIT for depth.
  void StoreColor(VkCommandBuffer command_buffer, VkFence fence,
                  VkImageView rt_image_view, ColorRenderTargetFormat rt_format,
                  MsaaSamples rt_samples, VkRect2D rt_rect,
                  uint32_t edram_offset_tiles, uint32_t edram_pitch_px);

  // Returns the maximum height of a render target in pixels.
  static uint32_t GetMaxHeight(bool format_64bpp, MsaaSamples samples,
                               uint32_t offset_tiles, uint32_t pitch_px);

  void Scavenge();

 private:
  enum class EDRAMImageStatus {
    kUntransitioned,
    kStore,
    kLoad
  };

  enum class Mode {
    k_ModeUnsupported = -1,

    // 32-bit color image format, non-multisampled.
    k_32bpp_1X,
    // 64-bit color image format, non-multisampled.
    k_64bpp_1X,

    k_ModeCount
  };

  struct ModeInfo {
    bool is_depth;
    bool is_64bpp;
    MsaaSamples samples;

    const uint8_t* store_shader_code;
    size_t store_shader_code_size;
    const char* store_shader_debug_name;
  };

  struct ModeData {
    // Store is compute.
    VkShaderModule store_shader_module = nullptr;
    VkPipeline store_pipeline = nullptr;
    // Load is graphics.
  };

  struct StorePushConstants {
    uint32_t edram_offset;
    uint32_t edram_pitch;
    uint32_t rt_offset[2];
  };

  struct LoadPushConstants {
    uint32_t edram_offset;
    uint32_t edram_pitch;
  };

  void TransitionEDRAMImage(VkCommandBuffer command_buffer, bool load);

  Mode GetColorMode(ColorRenderTargetFormat format, MsaaSamples samples);

  // Returns log2 of how many EDRAM image texels one framebuffer pixel covers.
  static void GetPixelEDRAMSizePower(bool format_64bpp, MsaaSamples samples,
                                     uint32_t& width_power,
                                     uint32_t& height_power);

  // Returns false if shouldn't or can't load or store this EDRAM portion.
  // Not necessarily in case of an error, returns false for 0x0 framebuffer too.
  // This assumes that the whole framebuffer starts at a whole tile.
  bool GetDimensions(bool format_64bpp, MsaaSamples samples,
                     uint32_t edram_base_offset_tiles, uint32_t edram_pitch_px,
                     VkRect2D rt_rect, VkRect2D& rt_rect_adjusted,
                     uint32_t& edram_add_offset_tiles,
                     VkExtent2D& edram_extent_tiles,
                     uint32_t& edram_pitch_tiles);

  ui::vulkan::VulkanDevice* device_ = nullptr;

  // Memory backing the 10 MB tile image.
  VkDeviceMemory edram_memory_ = nullptr;
  // 1280x2048 image storing EDRAM tiles.
  VkImage edram_image_ = nullptr;
  // View of the EDRAM image.
  VkImageView edram_image_view_ = nullptr;
  // The current access mode for the EDRAM image.
  EDRAMImageStatus edram_image_status_ = EDRAMImageStatus::kUntransitioned;

  // Store pipeline layout.
  // 2 storage descriptor (EDRAM, render target for imageLoad).
  VkDescriptorSetLayout store_descriptor_set_layout_storage_color_ = nullptr;
  // 1 storage descriptor (EDRAM).
  VkDescriptorSetLayout store_descriptor_set_layout_storage_depth_ = nullptr;
  // 2 sampled descriptors (depth, stencil for texelFetch).
  VkDescriptorSetLayout store_descriptor_set_layout_sampled_depth_ = nullptr;
  VkPipelineLayout store_pipeline_layout_color_ = nullptr;
  VkPipelineLayout store_pipeline_layout_depth_ = nullptr;

  // Load pipeline layout.
  // 1 storage descriptor (EDRAM).
  VkDescriptorSetLayout load_descriptor_set_layout_storage_ = nullptr;
  VkPipelineLayout load_pipeline_layout_ = nullptr;

  // Descriptor pool for shader invocations.
  std::unique_ptr<ui::vulkan::DescriptorPool> descriptor_pool_ = nullptr;

  // Data for setting up each mode.
  static const ModeInfo mode_info_[Mode::k_ModeCount];
  // Mode-dependent data (load/store pipelines and per-mode dependencies).
  ModeData mode_data_[Mode::k_ModeCount];
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_EDRAM_STORE_H_
