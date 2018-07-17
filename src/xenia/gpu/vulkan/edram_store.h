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

#include "xenia/gpu/xenos.h"
#include "xenia/ui/vulkan/fenced_pools.h"
#include "xenia/ui/vulkan/vulkan.h"
#include "xenia/ui/vulkan/vulkan_device.h"

namespace xe {
namespace gpu {
namespace vulkan {

// Stores the raw contents of the EDRAM, basically manages guest framebuffers.
//
// =============================================================================
// How the EDRAM is used by Xenos:
// (Copied from the old version of the render target cache, so implementation
//  info may differ from the way EDRAM is emulated now.)
// =============================================================================
//
// On the 360 the render target is an opaque block of memory in EDRAM that's
// only accessible via resolves. We use this to our advantage to simulate
// something like it as best we can by having a shared backing memory with
// a multitude of views for each tile location in EDRAM.
//
// This allows us to have the same base address write to the same memory
// regardless of framebuffer format. Resolving then uses whatever format the
// resolve requests straight from the backing memory.
//
// EDRAM is a beast and we only approximate it as best we can. Basically,
// the 10MiB of EDRAM is composed of 2048 5120b tiles. Each tile is 80x16px.
// +-----+-----+-----+---
// |tile0|tile1|tile2|...  2048 times
// +-----+-----+-----+---
// Operations dealing with EDRAM deal in tile offsets, so base 0x100 is tile
// offset 256, 256*5120=1310720b into the buffer. All rendering operations are
// aligned to tiles so trying to draw at 256px wide will have a real width of
// 320px by rounding up to the next tile.
//
// MSAA and other settings will modify the exact pixel sizes, like 4X makes
// each tile effectively 40x8px / 2X makes each tile 80x8px, but they are still
// all 5120b. As we try to emulate this we adjust our viewport when rendering to
// stretch pixels as needed.
//
// It appears that games also take advantage of MSAA stretching tiles when doing
// clears. Games will clear a view with 1/2X pitch/height and 4X MSAA and then
// later draw to that view with 1X pitch/height and 1X MSAA.
//
// The good news is that games cannot read EDRAM directly but must use a copy
// operation to get the data out. That gives us a chance to do whatever we
// need to (re-tile, etc) only when requested.
//
// To approximate the tiled EDRAM layout we use a single large chunk of memory.
// From this memory we create many VkImages (and VkImageViews) of various
// formats and dimensions as requested by the game. These are used as
// attachments during rendering and as sources during copies. They are also
// heavily aliased - lots of images will reference the same locations in the
// underlying EDRAM buffer. The only requirement is that there are no hazards
// with specific tiles (reading/writing the same tile through different images)
// and otherwise it should be ok *fingers crossed*.
//
// One complication is the copy/resolve process itself: we need to give back
// the data asked for in the format desired and where it goes is arbitrary
// (any address in physical memory). If the game is good we get resolves of
// EDRAM into fixed base addresses with scissored regions. If the game is bad
// we are broken.
//
// Resolves from EDRAM result in tiled textures - that's texture tiles, not
// EDRAM tiles. If we wanted to ensure byte-for-byte correctness we'd need to
// then tile the images as we wrote them out. For now, we just attempt to
// get the (X, Y) in linear space and do that. This really comes into play
// when multiple resolves write to the same texture or memory aliased by
// multiple textures - which is common due to predicated tiling. The examples
// below demonstrate what this looks like, but the important thing is that
// we are aware of partial textures and overlapping regions.
//
// Example with multiple render targets:
//   Two color targets of 256x256px tightly packed in EDRAM:
//     color target 0: base 0x0, pitch 320, scissor 0,0, 256x256
//       starts at tile 0, buffer offset 0
//       contains 64 tiles (320/80)*(256/16)
//     color target 1: base 0x40, pitch 320, scissor 256,0, 256x256
//       starts at tile 64 (after color target 0), buffer offset 327680b
//       contains 64 tiles
//   In EDRAM each set of 64 tiles is contiguous:
//     +------+------+   +------+------+------+
//     |ct0.0 |ct0.1 |...|ct0.63|ct1.0 |ct1.1 |...
//     +------+------+   +------+------+------+
//   To render into these, we setup two VkImages:
//     image 0: bound to buffer offset 0, 320x256x4=327680b
//     image 1: bound to buffer offset 327680b, 320x256x4=327680b
//   So when we render to them:
//     +------+-+ scissored to 256x256, actually 320x256
//     | .    | | <- . appears at some untiled offset in the buffer, but
//     |      | |      consistent if aliased with the same format
//     +------+-+
//   In theory, this gives us proper aliasing in most cases.
//
// Example with horizontal predicated tiling:
//   Trying to render 1024x576 @4X MSAA, splitting into two regions
//   horizontally:
//     +----------+
//     | 1024x288 |
//     +----------+
//     | 1024x288 |
//     +----------+
//   EDRAM configured for 1056x288px with tile size 2112x567px (4X MSAA):
//     color target 0: base 0x0, pitch 1080, 26x36 tiles
//   First render (top):
//     window offset 0,0
//     scissor 0,0, 1024x288
//   First resolve (top):
//     RB_COPY_DEST_BASE    0x1F45D000
//     RB_COPY_DEST_PITCH   pitch=1024, height=576
//     vertices: 0,0, 1024,0, 1024,288
//   Second render (bottom):
//     window offset 0,-288
//     scissor 0,288, 1024x288
//   Second resolve (bottom):
//     RB_COPY_DEST_BASE    0x1F57D000 (+1179648b)
//     RB_COPY_DEST_PITCH   pitch=1024, height=576
//       (exactly 1024x288*4b after first resolve)
//     vertices: 0,288, 1024,288, 1024,576
//   Resolving here is easy as the textures are contiguous in memory. We can
//   snoop in the first resolve with the dest height to know the total size,
//   and in the second resolve see that it overlaps and place it in the
//   existing target.
//
// Example with vertical predicated tiling:
//   Trying to render 1280x720 @2X MSAA, splitting into two regions
//   vertically:
//     +-----+-----+
//     | 640 | 640 |
//     |  x  |  x  |
//     | 720 | 720 |
//     +-----+-----+
//   EDRAM configured for 640x736px with tile size 640x1472px (2X MSAA):
//     color target 0: base 0x0, pitch 640, 8x92 tiles
//   First render (left):
//     window offset 0,0
//     scissor 0,0, 640x720
//   First resolve (left):
//     RB_COPY_DEST_BASE    0x1BC6D000
//     RB_COPY_DEST_PITCH   pitch=1280, height=720
//     vertices: 0,0, 640,0, 640,720
//   Second render (right):
//     window offset -640,0
//     scissor 640,0, 640x720
//   Second resolve (right):
//     RB_COPY_DEST_BASE    0x1BC81000 (+81920b)
//     RB_COPY_DEST_PITCH   pitch=1280, height=720
//     vertices: 640,0, 1280,0, 1280,720
//   Resolving here is much more difficult as resolves are tiled and the right
//   half of the texture is 81920b away:
//     81920/4bpp=20480px, /32 (texture tile size)=640px
//   We know the texture size with the first resolve and with the second we
//   must check for overlap then compute the offset (in both X and Y).
//
// =============================================================================
// Current implementation details:
// =============================================================================
//
// The EDRAM contents are stored in a 1280x2048x2 image using compute shaders.
// The 1280x2048 size is chosen for easier debugging in RenderDoc and also to
// make the storage a bit more cache-friendly.
//
// Thanks to the fact that the EDRAM is not directly accessible by the CPU or
// shaders, we don't have to emulate whatever swizzling there may be in the
// EDRAM. Instead, we assume that framebuffers are stored there linearly, and
// the image is structured as 1280x16 rows of 16 tiles (tile index >> 4 * 16 and
// tile index & 15 * 80 can be used to get the coordinates of a specific tile).
//
// Color data is stored in the first layer in the guest-native format. During
// storing and loading, necessary conversion (such as between host float16 and
// guest 7e3.7e3.7e3.unorm2 for 2_10_10_10_FLOAT) is performed.
//
// Depth data is stored in two formats. In the first layer, it's stored in the
// guest format (24-bit unorm or 20e4 float) along with stencil data, and in the
// second, it's stored in the host 32-bit floating-point format. This is done to
// ensure precision invariance across multiple passes - if the game draws
// geometry in multiple layers, after dropping 8 bits, the depth test will fail
// a lot in the places where it should pass).
//
// Due to inability to reliably obtain framebuffer or even modified area height
// (especially when drawing rectangle lists without viewport scale and scissor),
// shaders are written in a way that if you don't draw anything between a load
// and a store, the originally loaded value will be written back - to ensure
// other framebuffers won't be corrupted if stores to them are overlapped (games
// also often cause overlap intentionally when clearing multiple buffers using a
// single rectangle, for example).
//
// Since there may be overlap between color and depth buffers as well, and
// drawing to either in this case is valid usage, it must also be ensured that
// the 32-bit depth value is kept up to date with the 24-bit one without
// dropping additional 8 bits of precision unless that's absolutely needed.
// Depth loads check whether the guest 24-bit depth value for each pixel is the
// same as if the host 32-bit value was converted to 24 bits, and if they're the
// same, load the more precise host value - otherwise, they give the new 24-bit
// value (which may be some color that must be preserved until some other pass).
// Depth stores and clears always write the new value to both layers.
//
// It should also be noted that if there's overlap, store calls must also be
// ordered from the smallest EDRAM base offset to the biggest.
//
// Due to Vulkan limitations, adding MSAA support to the EDRAM store would be
// pretty difficult and messy - loads would be fragment shaders rather than
// compute ones, writing to gl_SampleMask and gl_FragDepth, stencil would
// require at least 4 passes (for every 2 bits of it) to be loaded, and every
// shader touching the EDRAM would have to be done in 3 versions, for each MSAA
// level.
//
// SSAA, however, works very well with the EDRAM store, even better than if no
// AA is used at all - supersampled framebuffers are stored and loaded exactly
// the same way as non-supersampled, even using all the same shaders that don't
// know about the AA level being used.
class EDRAMStore {
 public:
  EDRAMStore(ui::vulkan::VulkanDevice* device);
  ~EDRAMStore();

  VkResult Initialize();
  void Shutdown();

  static inline bool IsGuestColorFormat64bpp(ColorRenderTargetFormat format) {
    return format == ColorRenderTargetFormat::k_16_16_16_16 ||
           format == ColorRenderTargetFormat::k_16_16_16_16_FLOAT ||
           format == ColorRenderTargetFormat::k_32_32_FLOAT;
  }

  VkFormat GetStoreColorImageViewFormat(ColorRenderTargetFormat format);

  // load = false to store the data to the EDRAM, load = true to load back.
  //
  // The image MUST have a width that is a multiple of 80 (or 40 for 64bpp guest
  // formats such as 16_16_16_16) and a height that is a multiple of 16,
  // otherwise crashes may happen.
  //
  // The image view must be in the R32_UINT format for images that are 32bpp on
  // the host, and R32G32_UINT for 64bpp (this includes 2_10_10_10_FLOAT images
  // emulated as 16_16_16_16_FLOAT, for instance). Obtain the correct format
  // using GetStoreColorImageViewFormat when creating the view.
  //
  // Prior to loading/storing, the render target must be in the following state:
  // StageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT.
  // AccessMask & VK_ACCESS_SHADER_READ_BIT for storing.
  // AccessMask & VK_ACCESS_SHADER_WRITE_BIT for loading.
  // Layout VK_IMAGE_LAYOUT_GENERAL.
  // It must be created with flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT and
  // usage & VK_IMAGE_USAGE_STORAGE_BIT.
  //
  // rt_rect_ss must be pre-supersampled.
  void CopyColor(VkCommandBuffer command_buffer, VkFence fence, bool load,
                 VkImageView rt_image_view_u32,
                 ColorRenderTargetFormat rt_format, MsaaSamples rt_samples,
                 VkRect2D rt_rect_ss, uint32_t edram_offset_tiles,
                 uint32_t edram_pitch_px);

  // load = false to store the data to the EDRAM, load = true to load back.
  //
  // The image MUST have a width that is a multiple of 80 and a height that is a
  // multiple of 16, otherwise crashes may happen.
  //
  // Prior to loading/storing, the depth image must be in the following state:
  // StageMask & VK_PIPELINE_STAGE_TRANSFER_BIT.
  // AccessMask & VK_ACCESS_TRANSFER_READ_BIT for storing.
  // AccessMask & VK_ACCESS_TRANSFER_WRITE_BIT for loading.
  // Layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL for storing.
  // Layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for loading.
  // It must be created with usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT for
  // storing and flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT for loading.
  //
  // rt_rect_ss must be pre-supersampled.
  void CopyDepth(VkCommandBuffer command_buffer, VkFence fence, bool load,
                 VkImage rt_image, DepthRenderTargetFormat rt_format,
                 MsaaSamples rt_samples, VkRect2D rt_rect_ss,
                 uint32_t edram_offset_tiles, uint32_t edram_pitch_px);

  void ClearColor(VkCommandBuffer command_buffer, VkFence fence,
                  bool format_64bpp, MsaaSamples samples,
                  uint32_t offset_tiles, uint32_t pitch_px,
                  uint32_t height_px, uint32_t color_high,
                  uint32_t color_low);

  void ClearDepth(VkCommandBuffer command_buffer, VkFence fence,
                  DepthRenderTargetFormat format, MsaaSamples samples,
                  uint32_t offset_tiles, uint32_t pitch_px,
                  uint32_t height_px, uint32_t stencil_depth);

  // Returns the non-supersampled maximum height of a render target in pixels.
  static uint32_t GetMaxHeight(bool format_64bpp, MsaaSamples samples,
                               uint32_t offset_tiles, uint32_t pitch_px);

  void Scavenge();

 private:
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
    // Packed 10.10.10.2 color with 7e3 float RGB and unorm alpha.
    k_7e3,

    // 24-bit normalized depth.
    k_D24,
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

  struct PushConstantsClear {
    uint32_t offset_tiles;
    uint32_t pitch_tiles;
    union {
      struct {
        uint32_t color_high;
        uint32_t color_low;
      };
      struct {
        uint32_t stencil_depth;
        uint32_t depth_host;
      };
    };
  };

  bool PrepareEDRAMImage(VkCommandBuffer command_buffer, VkFence fence);
  void TransitionDepthCopyBuffer(VkCommandBuffer command_buffer,
                                 DepthCopyBufferState new_state);

  Mode GetColorMode(ColorRenderTargetFormat format);
  Mode GetDepthMode(DepthRenderTargetFormat format);

  // Returns false if shouldn't or can't load or store this EDRAM portion.
  // Not necessarily in case of an error, returns false for 0x0 framebuffer too.
  // This assumes that the whole framebuffer starts at a whole tile.
  bool GetDimensions(bool format_64bpp, MsaaSamples samples,
                     uint32_t edram_base_offset_tiles, uint32_t edram_pitch_px,
                     VkRect2D rt_rect, VkRect2D& rt_rect_adjusted,
                     uint32_t& edram_add_offset_tiles,
                     VkExtent2D& edram_extent_tiles,
                     uint32_t& edram_pitch_tiles);

  void CommitEDRAMImageWrite(VkCommandBuffer command_buffer);

  static constexpr uint32_t kTotalTexelCount = 80 * 16 * 2048;

  ui::vulkan::VulkanDevice* device_ = nullptr;

  // Memory backing the 20 MB tile and host depth image array.
  VkDeviceMemory edram_memory_ = nullptr;
  // 1280x2048 image storing EDRAM tiles in layer 0 and 32-bit depth in layer 1.
  VkImage edram_image_ = nullptr;
  // View of the EDRAM image.
  VkImageView edram_image_view_ = nullptr;
  // Whether the EDRAM image has been transitioned before the first use.
  bool edram_image_prepared_ = false;

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
  VkDescriptorSetLayout descriptor_set_layout_color_ = nullptr;
  VkDescriptorSetLayout descriptor_set_layout_depth_ = nullptr;
  VkDescriptorSetLayout descriptor_set_layout_clear_ = nullptr;
  VkPipelineLayout pipeline_layout_color_ = nullptr;
  VkPipelineLayout pipeline_layout_depth_ = nullptr;
  VkPipelineLayout pipeline_layout_clear_ = nullptr;

  // Descriptor pool for shader invocations.
  std::unique_ptr<ui::vulkan::DescriptorPool> descriptor_pool_ = nullptr;

  // Data for setting up each mode.
  static const ModeInfo mode_info_[Mode::k_ModeCount];
  // Mode-dependent data (load/store pipelines and per-mode dependencies).
  ModeData mode_data_[Mode::k_ModeCount];

  // Clear pipelines.
  VkShaderModule clear_color_shader_module_ = nullptr;
  VkPipeline clear_color_pipeline_ = nullptr;
  VkShaderModule clear_depth_shader_module_ = nullptr;
  VkPipeline clear_depth_pipeline_ = nullptr;
};

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_VULKAN_EDRAM_STORE_H_
