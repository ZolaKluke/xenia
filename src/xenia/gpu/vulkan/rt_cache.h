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

#include <unordered_multimap>

#include "xenia/gpu/register_file.h"
#include "xenia/gpu/vulkan/edram_store.h"
#include "xenia/ui/vulkan/vulkan.h"
#include "xenia/ui/vulkan/vulkan_device.h"

namespace xe {
namespace gpu {
namespace vulkan {

// Manages the host memory for framebuffers, largely disregarding the EDRAM
// contents (the EDRAM store is used to preserve these).
// Framebuffers are allocated in 4 MB pages from up to 5 blocks 24 MB each.
// 24 MB is chosen because framebuffers on the Xenos can't be larger than 10 MB,
// but in some cases they are emulated with twice as large framebuffers here,
// and because of padding they may be bigger than 20 MB. Also Direct3D 12
// requires 4 MB alignment for MSAA framebuffers, and Vulkan is likely to do the
// same on Windows.
// 5 blocks is for the most extreme case (not practically possible though) with
// four 20 MB render targets and a depth/stencil buffer. Such amount of memory
// will likely never be allocated.
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
    void OnFrameEnd(VkCommandBuffer command_buffer, VkFence batch_fence);

    void ClearCache();
    void Scavenge();

    static ColorRenderTargetFormat GetBaseRTFormat(
        ColorRenderTargetFormat format);
    static VkFormat ColorRenderTargetFormatToVkFormat(
        ColorRenderTargetFormat format);
    static VkFormat DepthRenderTargetFormatToVkFormat(
        DepthRenderTargetFormat format);

  private:
    // Key used to index render targets bound to various 4 MB pages.
    union RenderTargetKey {
      struct {
        uint32_t width_div_80 : 6;  // 6
        uint32_t height_div_16 : 8;  // 14
        uint32_t is_depth : 1;  // 15
        union {
          ColorRenderTargetFormat color_format : 4;
          DepthRenderTargetFormat depth_format : 1;
        };  // 19
        MsaaSamples samples : 2;  // 21
      }
      uint32_t value;
      inline RenderTargetKey() : value(0) {}
      inline RenderTargetKey(const RenderTargetKey& key) : value(key.value) {}
      inline RenderTargetKey& operator=(const RenderTargetKey& key) {
        value = key.value;
        return *this;
      }
    };

    // One render target bound to a specific page.
    struct RenderTarget {
      VkImage image;
      VkImageView image_view;

      RenderTargetKey key;

      // Number of the first 4 MB page aliased by this render target.
      uint32_t page_first;
      // Number of 4 MB pages this render target uses.
      // Up to 6 - pages can't span multiple memory areas.
      uint32_t page_count;
    };

    // Finds or creates views for the specified render target configuration.
    // Pass zero keys for unused render targets.
    // Returns true if succeeded, false in case of an error.
    bool AllocateRenderTargets(const RenderTargetKey keys_color[4],
                               RenderTargetKey key_depth,
                               RenderTarget* rts_color[4],
                               RenderTarget** rt_depth);

    RegisterFile* register_file_ = nullptr;
    ui::vulkan::VulkanDevice* device_ = nullptr;

    // Storage for the preserving EDRAM contents across different views.
    EDRAMStore edram_store_;

    // 24 MB memory blocks backing render targets.
    VkDeviceMemory rt_memory_[5] = {};

    // Render target views indexed with render target keys.
    std::unordered_multimap<uint32_t, RenderTarget*> rts_;
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_RT_CACHE_H_
#endif  // RENDER_CACHE_NOT_OBSOLETE
