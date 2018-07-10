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

#include <unordered_map>

#include "xenia/gpu/registers.h"
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
      // Either failed to enter a render pass, or drawing will have no effect.
      kDoNotDraw,
      // Started a new Vulkan render pass - need to resubmit state.
      kDrawInNewPass,
      // Still drawing in the same render pass - current state still valid.
      kDrawInSamePass
    };

    RTCache(RegisterFile* register_file, ui::vulkan::VulkanDevice* device);
    ~RTCache();

    VkResult Initialize();
    void Shutdown();

    // Returns whether a new render pass has started and need to rebind things.
    DrawStatus OnDraw(VkCommandBuffer command_buffer, VkFence batch_fence);
    void OnFrameEnd(VkCommandBuffer command_buffer, VkFence batch_fence);
    VkRenderPass GetCurrentVulkanRenderPass();

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
    // Zero key means the render target is not used.
    union RenderTargetKey {
      struct {
        uint32_t width_div_80 : 6;  // 6
        uint32_t height_div_16 : 8;  // 14
        uint32_t is_depth : 1;  // 15
        // ColorRenderTargetFormat or DepthRenderTargetFormat.
        uint32_t format : 4;  // 19
        MsaaSamples samples : 2;  // 21
      };
      uint32_t value;
      inline RenderTargetKey() : value(0) {}
      inline RenderTargetKey(const RenderTargetKey& key) : value(key.value) {}
      inline RenderTargetKey& operator=(const RenderTargetKey& key) {
        value = key.value;
        return *this;
      }
    };

    enum class RenderTargetUsage {
      // Newly created.
      kUntransitioned,
      // Currently used for rendering or EDRAM loading.
      kFramebuffer,
      // Currently being stored to the EDRAM.
      kStoreToEDRAM,
      // Currently being loaded from the EDRAM.
      kLoadFromEDRAM
    };

    // One render target bound to a specific page.
    struct RenderTarget {
      VkImage image;
      VkImageView image_view;
      VkImageView image_view_stencil;
      VkImageView image_view_color_edram_store;

      RenderTargetKey key;

      // Number of the first 4 MB page aliased by this render target.
      uint32_t page_first;
      // Number of 4 MB pages this render target uses.
      // Up to 6 - pages can't span multiple memory areas.
      uint32_t page_count;

      RenderTargetUsage current_usage;
    };

    struct RenderPass {
      // Attachment 0 for depth if used, then color.
      VkRenderPass pass;
      VkFramebuffer framebuffer;

      // nullptr if not used.
      RenderTarget* rts_color[4];
      RenderTarget* rt_depth;

      // Cache optimization for search.
      RenderTargetKey keys_color[4];
      RenderTargetKey key_depth;

      // Dimensions for render area.
      uint32_t width;
      uint32_t height;
    };

    static inline void GetSupersampledSize(uint32_t& width, uint32_t& height,
                                           MsaaSamples samples) {
      if (samples >= MsaaSamples::k2X) {
        height *= 2;
        if (samples >= MsaaSamples::k4X) {
          width *= 2;
        }
      }
    }

    static constexpr VkImageUsageFlags kUsageFlagsColor =
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    static constexpr VkImageUsageFlags kUsageFlagsDepth =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    // Finds or creates views for the specified render target configuration.
    // Returns true if succeeded, false in case of an error.
    bool AllocateRenderTargets(const RenderTargetKey keys_color[4],
                               RenderTargetKey key_depth,
                               RenderTarget* rts_color[4],
                               RenderTarget** rt_depth);

    static VkPipelineStageFlags GetRenderTargetUsageParameters(
        bool is_depth, RenderTargetUsage usage, VkAccessFlags& access_mask,
        VkImageLayout& layout);

    // Finds or creates a render pass. Returns nullptr in case of an error.
    RenderPass* GetRenderPass(const RenderTargetKey keys_color[4],
                              RenderTargetKey key_depth);

    void SwitchRenderPassTargetUsage(VkCommandBuffer command_buffer,
                                     RenderPass* pass, RenderTargetUsage usage,
                                     uint32_t switch_color_mask,
                                     bool switch_depth);
    void BeginRenderPass(VkCommandBuffer command_buffer, VkFence batch_fence,
                         RenderPass* pass);
    void EndRenderPass(VkCommandBuffer command_buffer, VkFence batch_fence);
    bool AreCurrentEDRAMParametersValid() const;

    RegisterFile* register_file_ = nullptr;
    ui::vulkan::VulkanDevice* device_ = nullptr;

    // Storage for the preserving EDRAM contents across different views.
    EDRAMStore edram_store_;

    // Memory types that can be used for render targets.
    uint32_t rt_memory_type_bits_;
    // 24 MB memory blocks backing render targets.
    VkDeviceMemory rt_memory_[5] = {};

    // Render target views indexed with render target keys.
    std::unordered_multimap<uint32_t, RenderTarget*> rts_;

    std::vector<RenderPass*> passes_;

    // Shadows of the registers that impact the render pass we choose.
    // If the registers don't change between passes, we can quickly reuse the
    // previous one.
    struct ShadowRegisters {
      reg::RB_MODECONTROL rb_modecontrol;
      reg::RB_SURFACE_INFO rb_surface_info;
      reg::RB_COLOR_INFO rb_color_info[4];
      uint32_t rb_color_mask;
      reg::RB_DEPTH_INFO rb_depth_info;
      uint32_t pa_sc_window_scissor_tl;
      uint32_t pa_sc_window_scissor_br;

      ShadowRegisters() { Reset(); }
      void Reset() { std::memset(this, 0, sizeof(*this)); }
    } shadow_registers_;
    bool SetShadowRegister(uint32_t* dest, uint32_t register_name);

    // Current state.
    RenderPass* current_pass_ = nullptr;
    uint32_t current_edram_pitch_px_;
    uint32_t current_edram_color_offsets_[4];
    uint32_t current_edram_depth_offset_;
    // current_shadow_valid_ is set to false when need to do full OnDraw logic.
    // This may happen after a copy command that ends the pass, for example.
    bool current_shadow_valid_ = false;
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_RT_CACHE_H_
#endif  // RENDER_CACHE_NOT_OBSOLETE
