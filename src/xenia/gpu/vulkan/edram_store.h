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

  VkFormat GetStoreColorImageViewFormat(ColorRenderTargetFormat format);

  // load = false to store the data to the EDRAM, load = true to load back.
  // Prior to loading/storing, the render target must be in the following state:
  // StageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT.
  // AccessMask & VK_ACCESS_SHADER_READ_BIT for storing.
  // AccessMask & VK_ACCESS_SHADER_WRITE_BIT for loading.
  // Layout VK_IMAGE_LAYOUT_GENERAL.
  // It must be created with flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT and
  // usage & VK_IMAGE_USAGE_STORAGE_BIT.
  // The image view must be in the R32_UINT format for 32bpp (on the host)
  // images, and R32G32_UINT for 64bpp (use GetStoreColorImageViewFormat).
  void CopyColor(VkCommandBuffer command_buffer, VkFence fence, bool load,
                 VkImageView rt_image_view_u32,
                 ColorRenderTargetFormat rt_format, MsaaSamples rt_samples,
                 VkRect2D rt_rect, uint32_t edram_offset_tiles,
                 uint32_t edram_pitch_px);
  // Prior to loading/storing, the depth image must be in the following state:
  // StageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT.
  // AccessMask & VK_ACCESS_TRANSFER_READ_BIT for storing.
  // AccessMask & VK_ACCESS_TRANSFER_WRITE_BIT for loading.
  // Layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL for storing.
  // Layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for loading.
  // It must be created with usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT for
  // storing and flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT for loading.
  void CopyDepth(VkCommandBuffer command_buffer, VkFence fence, bool load,
                 VkImage rt_image, DepthRenderTargetFormat rt_format,
                 MsaaSamples rt_samples, VkRect2D rt_rect,
                 uint32_t edram_offset_tiles, uint32_t edram_pitch_px);

  void ClearColor(VkCommandBuffer command_buffer, VkFence fence,
                  bool format_64bpp, MsaaSamples samples,
                  uint32_t offset_tiles, uint32_t pitch_px,
                  uint32_t height_px, uint32_t color_high,
                  uint32_t color_low);
  void ClearDepth(VkCommandBuffer command_buffer, VkFence fence,
                  MsaaSamples samples, uint32_t offset_tiles, uint32_t pitch_px,
                  uint32_t height_px, uint32_t stencil_depth);

  // Returns the maximum height of a render target in pixels.
  static uint32_t GetMaxHeight(bool format_64bpp, MsaaSamples samples,
                               uint32_t offset_tiles, uint32_t pitch_px);

  void Scavenge();

 private:
  enum class EDRAMImageState {
    kUntransitioned,
    kStore,
    kLoad
  };

  enum class DepthCopyBufferState {
    kUntransitioned,
    kRenderTargetToBuffer,
    kBufferToEDRAM,
    kEDRAMToBuffer,
    kBufferToRenderTarget
  };

  enum class Mode {
    k_ModeUnsupported = -1,

    // 32-bit color.
    k_32bpp,
    // 64-bit color.
    k_64bpp,
    // Packed 10.10.10.2 float.
    k_7e3,

    // 24-bit normalized depth.
    // k_D24,
    // 20e4 floating-point depth.
    k_D24F,

    k_ModeCount
  };

  struct ModeInfo {
    bool is_depth;
    bool is_64bpp;

    const uint8_t* store_shader_code;
    size_t store_shader_code_size;
    const char* store_shader_debug_name;

    const uint8_t* load_shader_code;
    size_t load_shader_code_size;
    const char* load_shader_debug_name;
  };

  struct ModeData {
    // Compute shaders and pipelines.
    VkShaderModule store_shader_module = nullptr;
    VkPipeline store_pipeline = nullptr;
    VkShaderModule load_shader_module = nullptr;
    VkPipeline load_pipeline = nullptr;
  };

  struct PushConstantsColor {
    uint32_t edram_offset_tiles;
    uint32_t edram_pitch_tiles;
    uint32_t rt_offset_px[2];
  };

  struct PushConstantsDepth {
    uint32_t edram_offset_tiles;
    uint32_t edram_pitch_tiles;
    uint32_t buffer_pitch_px;
  };

  struct PushConstantsClearColor {
    uint32_t offset_tiles;
    uint32_t pitch_tiles;
    uint32_t color_high;
    uint32_t color_low;
  };

  void TransitionEDRAMImage(VkCommandBuffer command_buffer, bool load);
  void TransitionDepthCopyBuffer(VkCommandBuffer command_buffer,
                                 DepthCopyBufferState new_state);

  Mode GetColorMode(ColorRenderTargetFormat format);
  Mode GetDepthMode(DepthRenderTargetFormat format);

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

  static constexpr uint32_t kTotalTexelCount = 80 * 16 * 2048;

  ui::vulkan::VulkanDevice* device_ = nullptr;

  // Memory backing the 10 MB tile image.
  VkDeviceMemory edram_memory_ = nullptr;
  // 1280x2048 image storing EDRAM tiles.
  VkImage edram_image_ = nullptr;
  // View of the EDRAM image.
  VkImageView edram_image_view_ = nullptr;
  // The current access mode for the EDRAM image.
  EDRAMImageState edram_image_state_ = EDRAMImageState::kUntransitioned;

  // Memory backing the depth copy buffer.
  VkDeviceMemory depth_copy_memory_ = nullptr;
  // Buffer for image<->buffer copies of depth and stencil (after depth).
  VkBuffer depth_copy_buffer_ = nullptr;
  // Views of the depth copy buffer.
  VkBufferView depth_copy_buffer_view_depth_ = nullptr;
  VkBufferView depth_copy_buffer_view_stencil_ = nullptr;
  // The current access mode for the depth copy buffer.
  DepthCopyBufferState depth_copy_buffer_state_ =
      DepthCopyBufferState::kUntransitioned;

  // Pipeline layouts.
  // Color store and load (one EDRAM image and one RT image) descriptor layout.
  VkDescriptorSetLayout descriptor_set_layout_color_ = nullptr;
  VkPipelineLayout pipeline_layout_color_ = nullptr;
  // Depth store and load (one EDRAM image and D/S buffers) descriptor layout.
  VkDescriptorSetLayout descriptor_set_layout_depth_ = nullptr;
  VkPipelineLayout pipeline_layout_depth_ = nullptr;
  // Color clear (EDRAM image only) descriptor layout.
  VkDescriptorSetLayout descriptor_set_layout_clear_color_ = nullptr;
  VkPipelineLayout pipeline_layout_clear_color_ = nullptr;

  // Descriptor pool for shader invocations.
  std::unique_ptr<ui::vulkan::DescriptorPool> descriptor_pool_ = nullptr;

  // Data for setting up each mode.
  static const ModeInfo mode_info_[Mode::k_ModeCount];
  // Mode-dependent data (load/store pipelines and per-mode dependencies).
  ModeData mode_data_[Mode::k_ModeCount];

  // Clear pipelines.
  VkShaderModule clear_color_shader_module_ = nullptr;
  VkPipeline clear_color_pipeline_ = nullptr;
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_EDRAM_STORE_H_
