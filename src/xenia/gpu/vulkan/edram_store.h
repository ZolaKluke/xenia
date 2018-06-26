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
#include "xenia/ui/vulkan/vulkan_device.h"

namespace xe {
namespace gpu {
namespace vulkan {

// Stores the raw contents of the EDRAM in a buffer implemented as a 1280x2048
// 32-bit image.
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

  void StoreColor(VkCommandBuffer command_buffer, VkFence fence,
                  VkImage rt_image, ColorRenderTargetFormat rt_format,
                  VkExtent2D rt_extents, uint32_t edram_offset_tiles,
                  uint32_t edram_pitch_tiles);

 private:
  enum class Mode {
    k32Bpp1x,  // 32-bit image format, non-multisampled.

    kModeCount
  };

  struct ModeInfo {
    const uint8_t* shader_code;
    size_t shader_code_size;
  };

  struct ModeData {
    VkShaderModule store_shader_module = nullptr;
    VkPipeline store_pipeline = nullptr;
  };

  struct PushContants {
    uint32_t edram_offset;
    uint32_t edram_pitch;
    uint32_t rt_offset[2];
  };

  ui::vulkan::VulkanDevice* device_ = nullptr;

  // Memory backing the 10 MB tile image.
  VkDeviceMemory edram_memory_ = nullptr;
  // 1280x2048 image storing EDRAM tiles.
  VkImage edram_image_ = nullptr;
  // Whether the EDRAM image was made an UAV in the first command buffer.
  bool edram_image_transitioned_ = false;

  // Descriptor set layout for the load and store pipelines.
  VkDescriptorSetLayout descriptor_set_layout_ = nullptr;
  // Layout for the load and store pipelines.
  VkPipelineLayout pipeline_layout_ = nullptr;

  // Data for setting up each mode.
  static const ModeInfo mode_info_[Mode::kModeCount];
  // Mode-dependent data (load/store pipelines and per-mode dependencies).
  ModeData mode_data_[Mode::kModeCount];
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_EDRAM_STORE_H_
