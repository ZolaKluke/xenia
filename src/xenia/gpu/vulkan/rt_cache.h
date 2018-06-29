#ifndef RENDER_CACHE_NOT_OBSOLETE
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_VULKAN_RT_CACHE_H_
#define XENIA_GPU_VULKAN_RT_CACHE_H_

#include "xenia/gpu/register_file.h"
#include "xenia/gpu/vulkan/edram_store.h"
#include "xenia/ui/vulkan/vulkan.h"
#include "xenia/ui/vulkan/vulkan_device.h"

namespace xe {
namespace gpu {
namespace vulkan {

class RTCache {
  public:
    // Result of OnDraw, how the command processor needs to respond.
    enum class DrawStatus {
      // Failed to enter a Xenos render pass - can't send commands to Vulkan.
      kNotInRenderPass,
      // Started a new Vulkan render pass - need to resubmit state.
      kNewRenderPass,
      // Still drawing in the same render pass - current state still valid.
      // Viewport and scissor could have changed though.
      kSameRenderPass
    };

    RTCache(RegisterFile* register_file, ui::vulkan::VulkanDevice* device);
    ~RTCache();

    VkResult Initialize();
    void Shutdown();

    // Returns whether a new render pass has started and need to rebind things.
    DrawStatus OnDraw(VkCommandBuffer command_buffer, VkFence batch_fence);
    void OnFrameEnd();

    void ClearCache();
    void Scavenge();

  private:
    RegisterFile* register_file_ = nullptr;
    ui::vulkan::VulkanDevice* device_ = nullptr;

    // Storage for the preserving EDRAM contents across different views.
    EDRAMStore edram_store_;
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_RT_CACHE_H_
#endif  // RENDER_CACHE_NOT_OBSOLETE
