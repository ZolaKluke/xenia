#ifndef RENDER_CACHE_NOT_OBSOLETE
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <algorithm>

#include "xenia/gpu/vulkan/rt_cache.h"
#include "xenia/base/assert.h"
#include "xenia/base/logging.h"
#include "xenia/base/math.h"

namespace xe {
namespace gpu {
namespace vulkan {

using xe::ui::vulkan::CheckResult;

ColorRenderTargetFormat RTCache::GetBaseRTFormat(
    ColorRenderTargetFormat format) {
  switch (format) {
    case ColorRenderTargetFormat::k_8_8_8_8_GAMMA:
      return ColorRenderTargetFormat::k_8_8_8_8;
    case ColorRenderTargetFormat::k_2_10_10_10_AS_16_16_16_16:
      return ColorRenderTargetFormat::k_2_10_10_10;
    case ColorRenderTargetFormat::k_2_10_10_10_FLOAT_AS_16_16_16_16:
      return ColorRenderTargetFormat::k_2_10_10_10_FLOAT;
    default:
      return format;
  }
}

VkFormat RTCache::ColorRenderTargetFormatToVkFormat(
    ColorRenderTargetFormat format) const {
  switch (format) {
    case ColorRenderTargetFormat::k_8_8_8_8:
    case ColorRenderTargetFormat::k_8_8_8_8_GAMMA:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case ColorRenderTargetFormat::k_2_10_10_10:
    case ColorRenderTargetFormat::k_2_10_10_10_AS_16_16_16_16:
      return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    case ColorRenderTargetFormat::k_2_10_10_10_FLOAT:
    case ColorRenderTargetFormat::k_2_10_10_10_FLOAT_AS_16_16_16_16:
      return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ColorRenderTargetFormat::k_16_16:
      return VK_FORMAT_R16G16_UNORM;
    case ColorRenderTargetFormat::k_16_16_16_16:
      return VK_FORMAT_R16G16B16A16_UNORM;
    case ColorRenderTargetFormat::k_16_16_FLOAT:
      return VK_FORMAT_R16G16_SFLOAT;
    case ColorRenderTargetFormat::k_16_16_16_16_FLOAT:
      return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ColorRenderTargetFormat::k_32_FLOAT:
      return VK_FORMAT_R32_SFLOAT;
    case ColorRenderTargetFormat::k_32_32_FLOAT:
      return VK_FORMAT_R32G32_SFLOAT;
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

VkFormat RTCache::DepthRenderTargetFormatToVkFormat(
    DepthRenderTargetFormat format) const {
  switch (format) {
    case DepthRenderTargetFormat::kD24S8:
      // TODO(Triang3l): Add VK_FORMAT_D24_UNORM_S8_UINT to the EDRAM Store.
      return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case DepthRenderTargetFormat::kD24FS8:
      // Vulkan doesn't support 24-bit floats, so just promote it to 32-bit.
      return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

RTCache::RTCache(RegisterFile* register_file,
                 ui::vulkan::VulkanDevice* device)
    : register_file_(register_file), device_(device), edram_store_(device) {}

RTCache::~RTCache() { Shutdown(); }

VkResult RTCache::Initialize() {
  VkResult status;

  // Initialize the EDRAM contents storage.
  status = edram_store_.Initialize();
  if (status != VK_SUCCESS) {
    return status;
  }

  // Get the usable memory types for the framebuffers.
  VkImageCreateInfo image_info;
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.pNext = nullptr;
  image_info.flags = 0;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
  image_info.extent.width = 1280;
  image_info.extent.height = 720;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_4_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = kRTImageUsageFlagsDepth;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.queueFamilyIndexCount = 0;
  image_info.pQueueFamilyIndices = nullptr;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  VkImage memory_check_image;
  status = vkCreateImage(*device_, &image_info, nullptr, &memory_check_image);
  CheckResult(status, "vkCreateImage");
  if (status != VK_SUCCESS) {
    return status;
  }
  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(*device_, memory_check_image,
                               &memory_requirements);
  vkDestroyImage(*device_, memory_check_image, nullptr);
  assert_true(memory_requirements.alignment <= (1 << 22));
  rt_memory_type_bits_ = memory_requirements.memoryTypeBits;

  current_shadow_valid_ = false;

  return VK_SUCCESS;
}

void RTCache::Shutdown() {
  // TODO(Triang3l): Wait for idle.

  for (auto pass : passes_) {
    vkDestroyFramebuffer(*device_, pass->framebuffer, nullptr);
    vkDestroyRenderPass(*device_, pass->pass, nullptr);
    delete pass;
  }
  passes_.clear();

  for (auto rt_pair : rts_) {
    auto rt = rt_pair.second;
    if (rt->image_view_color_edram_store != nullptr) {
      vkDestroyImageView(*device_, rt->image_view_color_edram_store, nullptr);
    }
    if (rt->image_view_stencil != nullptr) {
      vkDestroyImageView(*device_, rt->image_view_stencil, nullptr);
    }
    vkDestroyImageView(*device_, rt->image_view, nullptr);
    vkDestroyImage(*device_, rt->image, nullptr);
    delete rt;
  }
  rts_.clear();

  for (size_t i = 0; i < xe::countof(rt_memory_); ++i) {
    VK_SAFE_DESTROY(vkFreeMemory, *device_, rt_memory_[i], nullptr);
  }

  edram_store_.Shutdown();
}

bool RTCache::IsRenderTargetKeyValid(RenderTargetKey key) const {
  if (key.width_div_80 == 0 || key.height_div_16 == 0) {
    return false;
  }
  return GetRenderTargetKeyVkFormat(key) != VK_FORMAT_UNDEFINED;
}

void RTCache::FillRenderTargetImageCreateInfo(
    RenderTargetKey key, VkImageCreateInfo& image_info) const {
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.pNext = nullptr;
  image_info.flags = key.is_depth ? 0 : VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = GetRenderTargetKeyVkFormat(key);
  image_info.extent.width = key.width_div_80 * 80;
  image_info.extent.height = key.height_div_16 * 16;
  GetSupersampledSize(image_info.extent.width, image_info.extent.height,
                      key.samples);
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = key.is_depth ? kRTImageUsageFlagsDepth :
                                    kRTImageUsageFlagsColor;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.queueFamilyIndexCount = 0;
  image_info.pQueueFamilyIndices = nullptr;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

RTCache::RenderTarget* RTCache::FindOrCreateRenderTarget(RenderTargetKey key,
                                                         uint32_t page_first) {
  // Check if there is already the needed render target.
  auto found_rts = rts_.equal_range(key.value);
  for (auto iter = found_rts.first; iter != found_rts.second; ++iter) {
    RenderTarget* found_rt = iter->second;
    if (found_rt->page_first == page_first) {
      return found_rt;
    }
  }

  VkResult status;

  // Create a new render target image.
  VkImageCreateInfo image_info;
  FillRenderTargetImageCreateInfo(key, image_info);
  VkImage image;
  status = vkCreateImage(*device_, &image_info, nullptr, &image);
  CheckResult(status, "vkCreateImage");
  if (status != VK_SUCCESS) {
    return false;
  }

  // Get the page count to store later.
  VkMemoryRequirements image_memory_requirements;
  vkGetImageMemoryRequirements(*device_, image, &image_memory_requirements);
  assert_always(image_memory_requirements.alignment <= (1 << 22));
  if (image_memory_requirements.size > (6 << 22)) {
    // Can't fit the image in a whole 24 MB block.
    assert_always();
    vkDestroyImage(*device_, image, nullptr);
    return false;
  }
  uint32_t page_count =
      xe::round_up(uint32_t(image_memory_requirements.size), 1u << 22) >> 22;
  uint32_t block_index = page_first / 6;
  uint32_t block_page_index = page_first - (block_index * 6);
  if (block_page_index + page_count > 6) {
    // Can't put the image at the requested position in the block.
    assert_always();
    vkDestroyImage(*device_, image, nullptr);
    return false;
  }

  // Name the image.
  // TODO(Triang3l): Color and depth format names.
  device_->DbgSetObjectName(
      uint64_t(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
      xe::format_string("RT(%c): %u, %ux, pages %u-%u",
                        key.is_depth ? 'd' : 'c', key.format,
                        1 << uint32_t(key.samples), page_first,
                        page_first + page_count));

  // Allocate the block if it doesn't exist yet.
  if (rt_memory_[block_index] == nullptr) {
    VkMemoryRequirements block_memory_requirements;
    block_memory_requirements.size = 6 << 22;
    block_memory_requirements.alignment = 1 << 22;
    block_memory_requirements.memoryTypeBits = rt_memory_type_bits_;
    // On the testing GTX 850M, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT is required.
    rt_memory_[block_index] =
        device_->AllocateMemory(block_memory_requirements,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (rt_memory_[block_index] == nullptr) {
      assert_always();
      vkDestroyImage(*device_, image, nullptr);
      return false;
    }
  }

  // Bind the memory to the image.
  status = vkBindImageMemory(*device_, image, rt_memory_[block_index],
                             block_page_index << 22);
  CheckResult(status, "vkBindImageMemory");
  if (status != VK_SUCCESS) {
    vkDestroyImage(*device_, image, nullptr);
    return false;
  }

  // Create the needed image views.
  VkImageViewCreateInfo image_view_info;
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.pNext = nullptr;
  image_view_info.flags = 0;
  image_view_info.image = image;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.format = image_info.format;
  image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
  image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
  image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
  image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
  image_view_info.subresourceRange.aspectMask =
      key.is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_info.subresourceRange.baseMipLevel = 0;
  image_view_info.subresourceRange.levelCount = 1;
  image_view_info.subresourceRange.baseArrayLayer = 0;
  image_view_info.subresourceRange.layerCount = 1;
  VkImageView image_view;
  status = vkCreateImageView(*device_, &image_view_info, nullptr, &image_view);
  CheckResult(status, "vkCreateImageView");
  if (status != VK_SUCCESS) {
    vkDestroyImage(*device_, image, nullptr);
    return false;
  }
  VkImageView image_view_stencil = nullptr;
  VkImageView image_view_color_edram_store = nullptr;
  if (key.is_depth) {
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    status = vkCreateImageView(*device_, &image_view_info, nullptr,
                               &image_view_stencil);
    CheckResult(status, "vkCreateImageView");
    if (status != VK_SUCCESS) {
      vkDestroyImageView(*device_, image_view, nullptr);
      vkDestroyImage(*device_, image, nullptr);
      return false;
    }
  } else {
    image_view_info.format = edram_store_.GetStoreColorImageViewFormat(
        ColorRenderTargetFormat(key.format));
    status = vkCreateImageView(*device_, &image_view_info, nullptr,
                               &image_view_color_edram_store);
    CheckResult(status, "vkCreateImageView");
    if (status != VK_SUCCESS) {
      vkDestroyImageView(*device_, image_view, nullptr);
      vkDestroyImage(*device_, image, nullptr);
      return false;
    }
  }

  // Add a new entry to the cache.
  RenderTarget* rt = new RenderTarget;
  rt->image = image;
  rt->image_view = image_view;
  rt->image_view_stencil = image_view_stencil;
  rt->image_view_color_edram_store = image_view_color_edram_store;
  rt->key.value = key.value;
  rt->page_first = page_first;
  rt->page_count = page_count;
  rt->current_usage = RenderTargetUsage::kUntransitioned;
  rts_.insert(std::make_pair(key.value, rt));
  return rt;
}

bool RTCache::AllocateRenderTargets(
    const RTCache::RenderTargetKey keys_color[4],
    RTCache::RenderTargetKey key_depth, RTCache::RenderTarget* rts_color[4],
    RTCache::RenderTarget** rt_depth) {
  // Since the logic is the same for color and depth, combine them.
  // Prefer placing in a more stable position (closer to the first page) so it
  // has less different views.
  RenderTargetKey keys[5];
  keys[0].value = key_depth.value;
  std::memcpy(&keys[1], keys_color, 4 * sizeof(RenderTargetKey));

  // Validate attachments.
  for (uint32_t i = 0; i < 5; ++i) {
    RenderTargetKey& key = keys[i];
    if (key.value == 0) {
      continue;
    }
    if (!IsRenderTargetKeyValid(key)) {
      assert_always();
      return false;
    }
    if (i == 0) {
      if (!key.is_depth) {
        assert_always();
        return false;
      }
    } else {
      if (key.is_depth) {
        assert_always();
        return false;
      }
      ColorRenderTargetFormat format = ColorRenderTargetFormat(key.format);
      format = GetBaseRTFormat(format);
      key.format = uint32_t(format);
    }
  }

  VkResult status;

  // Find page count for each render target.
  struct RTAllocInfo {
    uint32_t rt_index;
    uint32_t page_first;
    uint32_t page_count;
  };
  RTAllocInfo alloc_infos[5];
  uint32_t used_rt_count = 0;
  for (uint32_t i = 0; i < 5; ++i) {
    RenderTargetKey key(keys[i]);
    if (key.value == 0) {
      continue;
    }
    RTAllocInfo& alloc_info = alloc_infos[used_rt_count];
    alloc_info.rt_index = i;
    auto found_rt_iter = rts_.find(key.value);
    if (found_rt_iter != rts_.end()) {
      // There is a render target with the requested parameters already.
      // Reusing its page count. It may be aliased, but this doesn't matter.
      alloc_info.page_count = found_rt_iter->second->page_count;
      ++used_rt_count;
      continue;
    }
    // Need a temporary VkImage to get the required memory size.
    VkImageCreateInfo size_image_info;
    VkImage size_image;
    FillRenderTargetImageCreateInfo(key, size_image_info);
    status = vkCreateImage(*device_, &size_image_info, nullptr, &size_image);
    CheckResult(status, "vkCreateImage");
    if (status != VK_SUCCESS) {
      return false;
    }
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(*device_, size_image, &memory_requirements);
    vkDestroyImage(*device_, size_image, nullptr);
    assert_always(memory_requirements.alignment <= (1 << 22));
    if (memory_requirements.size > (6 << 22)) {
      // Can't fit the image in a whole 24 MB block.
      assert_always();
      return false;
    }
    alloc_info.page_count =
        xe::round_up(uint32_t(memory_requirements.size), 1u << 22) >> 22;
    ++used_rt_count;
  }
  if (used_rt_count == 0) {
    std::memset(rts_color, 0, 4 * sizeof(RenderTarget*));
    *rt_depth = nullptr;
    return true;
  }

  // Try to pack the framebuffer 4 MB pages tightly.
  // Start with the largest, they may jump across blocks first, creating holes.
  // Then fill the holes with smaller framebuffers.
  std::sort(alloc_infos, alloc_infos + used_rt_count,
            [](RTAllocInfo a, RTAllocInfo b) {
              if (a.page_count > b.page_count) {
                return true;
              }
              return a.rt_index < b.rt_index;
            });
  // Number of pages allocated in each 24 MB block.
  uint32_t pages_allocated[5] = {};
  for (uint32_t i = 0; i < used_rt_count; ++i) {
    RTAllocInfo& alloc_info = alloc_infos[i];
    uint32_t block_index;
    for (block_index = 0; block_index < 5; ++block_index) {
      if (pages_allocated[block_index] + alloc_info.page_count <= 6) {
        break;
      }
    }
    if (block_index >= 5) {
      // Couldn't find a block - this must not happen as there are 5.
      assert_always();
      return false;
    }
    alloc_info.page_first = block_index * 6 + pages_allocated[block_index];
    pages_allocated[block_index] += alloc_info.page_count;
  }

  // Find or create the needed render targets.
  RenderTarget* rts[5] = {};
  for (uint32_t i = 0; i < used_rt_count; ++i) {
    const RTAllocInfo& alloc_info = alloc_infos[i];
    uint32_t rt_index = alloc_info.rt_index;
    rts[rt_index] = FindOrCreateRenderTarget(keys[rt_index],
                                             alloc_info.page_first);
    if (rts[rt_index] == nullptr) {
      return false;
    }
  }

  // Return the found or created render targets.
  *rt_depth = rts[0];
  std::memcpy(rts_color, &rts[1], 4 * sizeof(RenderTarget*));
  return true;
}

VkPipelineStageFlags RTCache::GetRenderTargetUsageParameters(
    bool is_depth, RenderTargetUsage usage, VkAccessFlags& access_mask,
    VkImageLayout& layout) {
  switch (usage) {
    case RenderTargetUsage::kUntransitioned:
      access_mask = 0;
      layout = VK_IMAGE_LAYOUT_UNDEFINED;
      return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    case RenderTargetUsage::kFramebuffer:
      if (is_depth) {
        access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
               VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      }
      access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case RenderTargetUsage::kStoreToEDRAM:
      if (is_depth) {
        access_mask = VK_ACCESS_TRANSFER_READ_BIT;
        layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
      }
      access_mask = VK_ACCESS_SHADER_READ_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
      return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case RenderTargetUsage::kLoadFromEDRAM:
      if (is_depth) {
        access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
      }
      access_mask = VK_ACCESS_SHADER_WRITE_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
      return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case RenderTargetUsage::kResolve:
      assert_false(is_depth);
      access_mask = VK_ACCESS_SHADER_READ_BIT;
      // TODO(Triang3l): Check if the blitter can be switched to using
      // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
      layout = VK_IMAGE_LAYOUT_GENERAL;
      return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    default:
      assert_unhandled_case(usage);
  };
  access_mask = 0;
  layout = VK_IMAGE_LAYOUT_GENERAL;
  return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

void RTCache::SwitchSingleRenderTargetUsage(VkCommandBuffer command_buffer,
                                            RenderTarget* rt,
                                            RenderTargetUsage usage) {
  if (rt->current_usage == usage) {
    return;
  }
  uint32_t is_depth = rt->key.is_depth;
  VkImageMemoryBarrier image_barrier;
  uint32_t image_barrier_count = 0;
  VkPipelineStageFlags stage_mask_src =
      GetRenderTargetUsageParameters(bool(is_depth), rt->current_usage,
                                     image_barrier.srcAccessMask,
                                     image_barrier.oldLayout);
  VkPipelineStageFlags stage_mask_dst =
      GetRenderTargetUsageParameters(bool(is_depth), usage,
                                     image_barrier.dstAccessMask,
                                     image_barrier.newLayout);
  if (image_barrier.srcAccessMask != image_barrier.dstAccessMask ||
      image_barrier.oldLayout != image_barrier.newLayout) {
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.pNext = nullptr;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = rt->image;
    image_barrier.subresourceRange.aspectMask =
        is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
                   VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    image_barrier_count = 1;
  }
  vkCmdPipelineBarrier(command_buffer, stage_mask_src, stage_mask_dst, 0, 0,
                       nullptr, 0, nullptr, image_barrier_count,
                       &image_barrier);
  rt->current_usage = usage;
}

RTCache::RenderPass* RTCache::GetRenderPass(
    const RTCache::RenderTargetKey keys_color[4],
    RTCache::RenderTargetKey key_depth) {
  // Check if there is an existing render pass with such render targets.
  // TODO(Triang3l): Use a better structure (though pass count is mostly small).
  for (auto existing_pass : passes_) {
    if (!std::memcmp(existing_pass->keys_color, keys_color,
                     4 * sizeof(RenderTargetKey))
        && existing_pass->key_depth.value == key_depth.value) {
      return existing_pass;
    }
  }

  // Obtain the attachments for the pass.
  RenderTarget* rts_color[4];
  RenderTarget* rt_depth;
  if (!AllocateRenderTargets(keys_color, key_depth, rts_color, &rt_depth)) {
    return false;
  }

  VkResult status;

  // Create a new Vulkan render pass.
  VkRenderPassCreateInfo pass_info;
  VkSubpassDescription subpass;
  VkAttachmentDescription attachments[5];
  VkImageView attachment_image_views[5];
  VkAttachmentReference color_attachments[4], depth_attachment;
  uint32_t width_div_80_min = UINT32_MAX, height_div_16_min = UINT32_MAX;
  pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  pass_info.pNext = nullptr;
  pass_info.flags = 0;
  pass_info.attachmentCount = 0;
  pass_info.pAttachments = attachments;
  pass_info.subpassCount = 1;
  pass_info.pSubpasses = &subpass;
  pass_info.dependencyCount = 0;
  pass_info.pDependencies = nullptr;
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.colorAttachmentCount = 4;
  subpass.pColorAttachments = color_attachments;
  subpass.pResolveAttachments = nullptr;
  subpass.pDepthStencilAttachment = nullptr;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;
  // Attachment 0 is depth, then color.
  if (rt_depth != nullptr) {
    subpass.pDepthStencilAttachment = &depth_attachment;
    depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.attachment = pass_info.attachmentCount;
    VkAttachmentDescription& attachment =
        attachments[pass_info.attachmentCount];
    attachment.flags = 0;
    attachment.format =
        DepthRenderTargetFormatToVkFormat(DepthRenderTargetFormat(
                                          key_depth.format));
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment_image_views[pass_info.attachmentCount] = rt_depth->image_view;
    uint32_t width_div_80 = key_depth.width_div_80;
    uint32_t height_div_16 = key_depth.height_div_16;
    GetSupersampledSize(width_div_80, height_div_16, key_depth.samples);
    width_div_80_min = std::min(width_div_80, width_div_80_min);
    height_div_16_min = std::min(height_div_16, height_div_16_min);
    ++pass_info.attachmentCount;
  }
  for (uint32_t i = 0; i < 4; ++i) {
    VkAttachmentReference& color_attachment = color_attachments[i];
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    RenderTarget* rt_color = rts_color[i];
    if (rt_color == nullptr) {
      color_attachment.attachment = VK_ATTACHMENT_UNUSED;
      continue;
    }
    RenderTargetKey key(keys_color[i]);
    color_attachment.attachment = pass_info.attachmentCount;
    VkAttachmentDescription& attachment =
        attachments[pass_info.attachmentCount];
    attachment.flags = 0;
    attachment.format = ColorRenderTargetFormatToVkFormat(
        ColorRenderTargetFormat(key.format));
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_image_views[pass_info.attachmentCount] = rt_color->image_view;
    uint32_t width_div_80 = key.width_div_80;
    uint32_t height_div_16 = key.height_div_16;
    GetSupersampledSize(width_div_80, height_div_16, key.samples);
    width_div_80_min = std::min(width_div_80, width_div_80_min);
    height_div_16_min = std::min(height_div_16, height_div_16_min);
    ++pass_info.attachmentCount;
  }
  VkRenderPass pass;
  status = vkCreateRenderPass(*device_, &pass_info, nullptr, &pass);
  CheckResult(status, "vkCreateRenderPass");
  if (status != VK_SUCCESS) {
    return nullptr;
  }

  // Create a framebuffer using the pass.
  VkFramebufferCreateInfo framebuffer_info;
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.pNext = nullptr;
  framebuffer_info.flags = 0;
  framebuffer_info.renderPass = pass;
  framebuffer_info.attachmentCount = pass_info.attachmentCount;
  framebuffer_info.pAttachments = attachment_image_views;
  if (width_div_80_min == UINT32_MAX && height_div_16_min == UINT32_MAX) {
    framebuffer_info.width = 80;
    framebuffer_info.height = 16;
  } else {
    framebuffer_info.width = width_div_80_min * 80;
    framebuffer_info.height = height_div_16_min * 16;
  }
  framebuffer_info.layers = 1;
  VkFramebuffer framebuffer;
  status = vkCreateFramebuffer(*device_, &framebuffer_info, nullptr,
                                        &framebuffer);
  CheckResult(status, "vkCreateFramebuffer");
  if (status != VK_SUCCESS) {
    vkDestroyRenderPass(*device_, pass, nullptr);
    return nullptr;
  }

  // Insert a new pass object.
  RenderPass* new_pass = new RenderPass;
  new_pass->pass = pass;
  new_pass->framebuffer = framebuffer;
  std::memcpy(new_pass->rts_color, rts_color, 4 * sizeof(RenderTarget*));
  new_pass->rt_depth = rt_depth;
  std::memcpy(new_pass->keys_color, keys_color, 4 * sizeof(RenderTargetKey));
  new_pass->key_depth = key_depth;
  new_pass->width = framebuffer_info.width;
  new_pass->height = framebuffer_info.height;
  passes_.push_back(new_pass);
  return new_pass;
}

bool RTCache::SetShadowRegister(uint32_t* dest, uint32_t register_name) {
  uint32_t value = register_file_->values[register_name].u32;
  if (*dest == value) {
    return false;
  }
  *dest = value;
  return true;
}

void RTCache::SwitchRenderPassTargetUsage(
    VkCommandBuffer command_buffer, RenderPass* pass, RenderTargetUsage usage,
    uint32_t switch_color_mask, bool switch_depth) {
  // Filter out targets already in the required usage.
  for (uint32_t i = 0; i < 4; ++i) {
    if (!(switch_color_mask & (uint32_t(1) << i))) {
      continue;
    }
    RenderTarget* rt = pass->rts_color[i];
    if (rt == nullptr || rt->current_usage == usage) {
      switch_color_mask &= ~(uint32_t(1) << i);
    }
  }
  if (switch_depth) {
    if (pass->rt_depth == nullptr || pass->rt_depth->current_usage == usage) {
      switch_depth = false;
    }
  }
  if (!switch_color_mask && !switch_depth) {
    return;
  }

  // Switch usage.
  VkPipelineStageFlags stage_mask_src = 0, stage_mask_dst = 0;
  VkImageMemoryBarrier image_barriers[6];
  uint32_t image_barrier_count = 0;
  if (switch_color_mask) {
    VkAccessFlags access_mask_new;
    VkImageLayout layout_new;
    stage_mask_dst |=
        GetRenderTargetUsageParameters(false, usage, access_mask_new,
                                       layout_new);
    for (uint32_t i = 0; i < 4; ++i) {
      if (!(switch_color_mask & (uint32_t(1) << i))) {
        continue;
      }
      RenderTarget* rt = pass->rts_color[i];
      VkImageMemoryBarrier& image_barrier = image_barriers[image_barrier_count];
      stage_mask_src |=
          GetRenderTargetUsageParameters(false, rt->current_usage,
                                         image_barrier.srcAccessMask,
                                         image_barrier.oldLayout);
      rt->current_usage = usage;
      if (image_barrier.srcAccessMask == access_mask_new &&
          image_barrier.oldLayout == layout_new) {
        continue;
      }
      image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      image_barrier.pNext = nullptr;
      image_barrier.dstAccessMask = access_mask_new;
      image_barrier.newLayout = layout_new;
      image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      image_barrier.image = rt->image;
      image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_barrier.subresourceRange.baseMipLevel = 0;
      image_barrier.subresourceRange.levelCount = 1;
      image_barrier.subresourceRange.baseArrayLayer = 0;
      image_barrier.subresourceRange.layerCount = 1;
      ++image_barrier_count;
    }
  }
  if (switch_depth) {
    RenderTarget* rt = pass->rt_depth;
    VkImageMemoryBarrier& image_barrier = image_barriers[image_barrier_count];
    stage_mask_src |=
        GetRenderTargetUsageParameters(true, rt->current_usage,
                                       image_barrier.srcAccessMask,
                                       image_barrier.oldLayout);
    stage_mask_dst |=
        GetRenderTargetUsageParameters(true, usage, image_barrier.dstAccessMask,
                                       image_barrier.newLayout);
    rt->current_usage = usage;
    if (image_barrier.srcAccessMask != image_barrier.dstAccessMask ||
        image_barrier.oldLayout != image_barrier.newLayout) {
      image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      image_barrier.pNext = nullptr;
      image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      image_barrier.image = rt->image;
      image_barrier.subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      image_barrier.subresourceRange.baseMipLevel = 0;
      image_barrier.subresourceRange.levelCount = 1;
      image_barrier.subresourceRange.baseArrayLayer = 0;
      image_barrier.subresourceRange.layerCount = 1;
      ++image_barrier_count;
    }
  }
  vkCmdPipelineBarrier(command_buffer, stage_mask_src, stage_mask_dst, 0, 0,
                       nullptr, 0, nullptr, image_barrier_count,
                       image_barriers);
}

void RTCache::BeginRenderPass(VkCommandBuffer command_buffer,
                              VkFence batch_fence, RTCache::RenderPass* pass) {
  // Store the current values.
  current_pass_ = pass;
  auto& regs = shadow_registers_;
  current_edram_pitch_px_ = regs.rb_surface_info.surface_pitch;
  for (uint32_t i = 0; i < 4; ++i) {
    current_edram_color_offsets_[i] = regs.rb_color_info[i].color_base;
  }
  current_edram_depth_offset_ = regs.rb_depth_info.depth_base;

  // Load the values from the EDRAM.
  SwitchRenderPassTargetUsage(command_buffer, current_pass_,
                              RenderTargetUsage::kLoadFromEDRAM, 0xF, true);
  VkRect2D rt_rect;
  rt_rect.offset.x = 0;
  rt_rect.offset.y = 0;
  rt_rect.extent.width = current_pass_->width;
  rt_rect.extent.height = current_pass_->height;
  for (uint32_t i = 0; i < 4; ++i) {
    RenderTarget* rt = current_pass_->rts_color[i];
    if (rt == nullptr) {
      continue;
    }
    RenderTargetKey key(current_pass_->keys_color[i]);
    uint32_t format = key.format;
    edram_store_.CopyColor(command_buffer, batch_fence, true,
                           rt->image_view_color_edram_store,
                           ColorRenderTargetFormat(format), key.samples,
                           rt_rect, current_edram_color_offsets_[i],
                           current_edram_pitch_px_);
  }
  RenderTarget* rt_depth = current_pass_->rt_depth;
  if (rt_depth != nullptr) {
    RenderTargetKey key(current_pass_->key_depth);
    uint32_t format = key.format;
    edram_store_.CopyDepth(command_buffer, batch_fence, true, rt_depth->image,
                           DepthRenderTargetFormat(format), key.samples,
                           rt_rect, current_edram_depth_offset_,
                           current_edram_pitch_px_);
  }

  // Enter the framebuffer drawing mode.
  SwitchRenderPassTargetUsage(command_buffer, pass,
                              RenderTargetUsage::kFramebuffer, 0xF, true);
  VkRenderPassBeginInfo begin_info;
  begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_info.pNext = nullptr;
  begin_info.renderPass = pass->pass;
  begin_info.framebuffer = pass->framebuffer;
  begin_info.renderArea.offset.x = 0;
  begin_info.renderArea.offset.y = 0;
  begin_info.renderArea.extent.width = pass->width;
  begin_info.renderArea.extent.height = pass->height;
  begin_info.clearValueCount = 0;
  begin_info.pClearValues = nullptr;
  vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void RTCache::EndRenderPass(VkCommandBuffer command_buffer,
                            VkFence batch_fence) {
  // Shadow registers NOT valid here - also called from OnFrameEnd!

  if (current_pass_ == nullptr) {
    return;
  }

  vkCmdEndRenderPass(command_buffer);

  // Export the framebuffers to the EDRAM store.
  // They are exported from the first in the EDRAM to the last, so in case there
  // is overlap between multiple framebuffers used in one pass, they won't
  // overwrite each other.
  // TODO(Triang3l): Calculate non-overlapping EDRAM areas during pass creation.
  SwitchRenderPassTargetUsage(command_buffer, current_pass_,
                              RenderTargetUsage::kStoreToEDRAM, 0xF, true);
  VkRect2D rt_rect;
  rt_rect.offset.x = 0;
  rt_rect.offset.y = 0;
  rt_rect.extent.width = current_pass_->width;
  rt_rect.extent.height = current_pass_->height;

  struct StoreRenderTarget {
    // -1 for depth (it's more transient than color, and may be aliased (haven't
    // seen such behavior in any game though) if a game doesn't need depth).
    // (Such aliasing would actually break non-overlapping region detection).
    int32_t rt_index;
    uint32_t edram_base;
  } store_rts[5];
  uint32_t store_rt_count = 0;
  if (current_pass_->rt_depth != nullptr) {
    StoreRenderTarget& store_rt = store_rts[store_rt_count++];
    store_rt.rt_index = -1;
    store_rt.edram_base = current_edram_depth_offset_;
  }
  for (int32_t i = 0; i < 4; ++i) {
    if (current_pass_->rts_color[i] != nullptr) {
      StoreRenderTarget& store_rt = store_rts[store_rt_count++];
      store_rt.rt_index = i;
      store_rt.edram_base = current_edram_color_offsets_[i];
    }
  }
  std::sort(store_rts, store_rts + store_rt_count,
            [](StoreRenderTarget a, StoreRenderTarget b) {
              if (a.edram_base < b.edram_base) {
                return true;
              }
              return a.rt_index < b.rt_index;
            });

  for (uint32_t i = 0; i < store_rt_count; ++i) {
    StoreRenderTarget& store_rt = store_rts[i];
    int32_t rt_index = store_rt.rt_index;
    if (rt_index < 0) {
      RenderTarget* rt = current_pass_->rt_depth;
      RenderTargetKey key(current_pass_->key_depth);
      uint32_t format = key.format;
      edram_store_.CopyDepth(command_buffer, batch_fence, false, rt->image,
                             DepthRenderTargetFormat(format), key.samples,
                             rt_rect, current_edram_depth_offset_,
                             current_edram_pitch_px_);
    } else {
      RenderTarget* rt = current_pass_->rts_color[rt_index];
      RenderTargetKey key(current_pass_->keys_color[rt_index]);
      uint32_t format = key.format;
      edram_store_.CopyColor(command_buffer, batch_fence, false,
                             rt->image_view_color_edram_store,
                             ColorRenderTargetFormat(format), key.samples,
                             rt_rect, current_edram_color_offsets_[rt_index],
                             current_edram_pitch_px_);
    }
  }

  current_pass_ = nullptr;
}

bool RTCache::AreCurrentEDRAMParametersValid() const {
  if (current_pass_ == nullptr) {
    return false;
  }
  auto& regs = shadow_registers_;
  if (current_edram_pitch_px_ != regs.rb_surface_info.surface_pitch) {
    return false;
  }
  for (uint32_t i = 0; i < 4; ++i) {
    if (!(regs.rb_color_mask & (0xF << (i * 4)))) {
      continue;
    }
    if (current_edram_color_offsets_[i] != regs.rb_color_info[i].color_base) {
      return false;
    }
  }
  if (current_edram_depth_offset_ != regs.rb_depth_info.depth_base) {
    return false;
  }
  return true;
}

RTCache::DrawStatus RTCache::OnDraw(VkCommandBuffer command_buffer,
                                    VkFence batch_fence) {
  // Check if registers influencing the choice have changed.
  auto& regs = shadow_registers_;
  bool dirty = !current_shadow_valid_;
  dirty |=
      SetShadowRegister(&regs.rb_modecontrol.value, XE_GPU_REG_RB_MODECONTROL);
  dirty |= SetShadowRegister(&regs.rb_surface_info.value,
                             XE_GPU_REG_RB_SURFACE_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color_info[0].value, XE_GPU_REG_RB_COLOR_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color_info[1].value, XE_GPU_REG_RB_COLOR1_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color_info[2].value, XE_GPU_REG_RB_COLOR2_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color_info[3].value, XE_GPU_REG_RB_COLOR3_INFO);
  dirty |= SetShadowRegister(&regs.rb_color_mask, XE_GPU_REG_RB_COLOR_MASK);
  dirty |=
      SetShadowRegister(&regs.rb_depth_info.value, XE_GPU_REG_RB_DEPTH_INFO);
  dirty |= SetShadowRegister(&regs.pa_sc_window_scissor_tl,
                             XE_GPU_REG_PA_SC_WINDOW_SCISSOR_TL);
  dirty |= SetShadowRegister(&regs.pa_sc_window_scissor_br,
                             XE_GPU_REG_PA_SC_WINDOW_SCISSOR_BR);
  if (!dirty) {
    return current_pass_ != nullptr ? DrawStatus::kDrawInSamePass :
                                      DrawStatus::kDoNotDraw;
  }
  current_shadow_valid_ = true;

  // Get the mode, used color render targets and the sample count.
  xenos::ModeControl mode_control = regs.rb_modecontrol.edram_mode;
  if (mode_control != xenos::ModeControl::kColorDepth &&
      mode_control != xenos::ModeControl::kDepth) {
    EndRenderPass(command_buffer, batch_fence);
    return DrawStatus::kDoNotDraw;
  }
  uint32_t color_mask =
      mode_control == xenos::ModeControl::kColorDepth ? regs.rb_color_mask : 0;
  MsaaSamples samples = regs.rb_surface_info.msaa_samples;

  // Calculate the width of the host render target.
  uint32_t width = regs.rb_surface_info.surface_pitch;
  if (width == 0) {
    EndRenderPass(command_buffer, batch_fence);
    return DrawStatus::kDoNotDraw;
  }
  width = std::min(width, 2560u);
  // Round up so there are less switches and to make EDRAM load/store safer.
  uint32_t width_div_80 = xe::round_up(width, 80) / 80;

  // Calculate the height of the render pass.
  // TODO(Triang3l): Use 120 or 240 increments from the viewport instead of max.
  // TODO(Triang3l): For max, use the real offset, 0 is just for early testing.
  bool any_64bpp = false;
  for (uint32_t i = 0; i < 4; ++i) {
    if (!(color_mask & (0xF << (i * 4)))) {
      continue;
    }
    if (EDRAMStore::IsColorFormat64bpp(regs.rb_color_info[i].color_format)) {
      any_64bpp = true;
      break;
    }
  }
  uint32_t height = EDRAMStore::GetMaxHeight(any_64bpp, samples, 0, width);
  if (height == 0) {
    EndRenderPass(command_buffer, batch_fence);
    return DrawStatus::kDoNotDraw;
  }
  uint32_t height_div_16 = xe::round_up(height, 16) / 16;

  // Get the keys for the render pass.
  RenderTargetKey keys_color[4], key_depth;
  for (uint32_t i = 0; i < 4; ++i) {
    RenderTargetKey& key_color = keys_color[i];
    if (!(color_mask & (0xF << (i * 4)))) {
      key_color.value = 0;
      continue;
    }
    key_color.width_div_80 = width_div_80;
    key_color.height_div_16 = height_div_16;
    key_color.is_depth = 0;
    ColorRenderTargetFormat color_format = regs.rb_color_info[i].color_format;
    key_color.format = uint32_t(color_format);
    key_color.samples = samples;
  }
  key_depth.width_div_80 = width_div_80;
  key_depth.height_div_16 = height_div_16;
  key_depth.is_depth = 1;
  DepthRenderTargetFormat depth_format = regs.rb_depth_info.depth_format;
  key_depth.format = uint32_t(depth_format);
  key_depth.samples = samples;

  // Check if we can keep using the old pass.
  if (current_pass_ != nullptr) {
    if (!std::memcmp(current_pass_->keys_color, keys_color,
                     4 * sizeof(RenderTargetKey))
        && current_pass_->key_depth.value == key_depth.value) {
      if (AreCurrentEDRAMParametersValid()) {
        return DrawStatus::kDrawInSamePass;
      }
      RenderPass* last_pass = current_pass_;
      EndRenderPass(command_buffer, batch_fence);
      BeginRenderPass(command_buffer, batch_fence, last_pass);
      return DrawStatus::kDrawInNewPass;
    }
  }

  EndRenderPass(command_buffer, batch_fence);

  // Find or create the render pass and enter it.
  RenderPass* pass = GetRenderPass(keys_color, key_depth);
  if (pass == nullptr) {
    // Not supported or there have been Vulkan errors, don't render.
    return DrawStatus::kDoNotDraw;
  }
  BeginRenderPass(command_buffer, batch_fence, pass);
  return DrawStatus::kDrawInNewPass;
}

void RTCache::OnFrameEnd(VkCommandBuffer command_buffer, VkFence batch_fence) {
  EndRenderPass(command_buffer, batch_fence);
}

VkRenderPass RTCache::GetCurrentVulkanRenderPass() {
  if (current_pass_ == nullptr) {
    return nullptr;
  }
  return current_pass_->pass;
}

VkImageView RTCache::LoadResolveImage(
    VkCommandBuffer command_buffer, VkFence batch_fence, uint32_t edram_base,
    uint32_t surface_pitch, MsaaSamples samples, bool is_depth, uint32_t format,
    VkExtent2D& image_size) {
  if (is_depth) {
    // TODO(Triang3l): Support depth resolving when depth loading is done.
    return nullptr;
  }

  // Calculate the image size.
  if (surface_pitch == 0) {
    return nullptr;
  }
  surface_pitch = std::min(surface_pitch, 2560u);
  uint32_t width_div_80 = xe::round_up(surface_pitch, 80) / 80;
  bool is_64bpp = false;
  if (!is_depth) {
    is_64bpp = EDRAMStore::IsColorFormat64bpp(ColorRenderTargetFormat(format));
  }
  uint32_t height = EDRAMStore::GetMaxHeight(is_64bpp, samples, 0,
                                             surface_pitch);
  if (height == 0) {
    return nullptr;
  }
  uint32_t height_div_16 = xe::round_up(height, 16) / 16;

  // Use any existing RT image with the needed parameters or create a new one.
  RenderTargetKey key;
  key.width_div_80 = width_div_80;
  key.height_div_16 = height_div_16;
  key.is_depth = uint32_t(is_depth);
  key.format = format;
  key.samples = samples;
  RenderTarget* rt;
  auto found_rt_iter = rts_.find(key.value);
  if (found_rt_iter != rts_.end()) {
    rt = found_rt_iter->second;
  } else {
    rt = FindOrCreateRenderTarget(key, 0);
    if (rt == nullptr) {
      return nullptr;
    }
  }

  // Load the EDRAM data and return the image in a state suitable for sampling.
  SwitchSingleRenderTargetUsage(command_buffer, rt,
                                RenderTargetUsage::kLoadFromEDRAM);
  VkRect2D rt_rect;
  rt_rect.offset.x = 0;
  rt_rect.offset.y = 0;
  rt_rect.extent.width = width_div_80 * 80;
  rt_rect.extent.height = height_div_16 * 16;
  GetSupersampledSize(rt_rect.extent.width, rt_rect.extent.height, samples);
  if (!is_depth) {
    edram_store_.CopyColor(command_buffer, batch_fence, true,
                           rt->image_view_color_edram_store,
                           ColorRenderTargetFormat(format), key.samples,
                           rt_rect, edram_base, surface_pitch);
  }
  SwitchSingleRenderTargetUsage(command_buffer, rt,
                                RenderTargetUsage::kResolve);
  image_size = rt_rect.extent;
  return rt->image_view;
}

void RTCache::ClearColor(VkCommandBuffer command_buffer, VkFence fence,
                         ColorRenderTargetFormat format, MsaaSamples samples,
                         uint32_t offset_tiles, uint32_t pitch_px,
                         uint32_t height_px, uint32_t color_high,
                         uint32_t color_low) {
  assert_false(current_pass_);
  edram_store_.ClearColor(command_buffer, fence,
                          EDRAMStore::IsColorFormat64bpp(format), samples,
                          offset_tiles, pitch_px, height_px, color_high,
                          color_low);
}

void RTCache::ClearDepth(VkCommandBuffer command_buffer, VkFence fence,
                         MsaaSamples samples, uint32_t offset_tiles,
                         uint32_t pitch_px, uint32_t height_px,
                         uint32_t stencil_depth) {
  assert_false(current_pass_);
  edram_store_.ClearDepth(command_buffer, fence, samples, offset_tiles,
                          pitch_px, height_px, stencil_depth);
}

void RTCache::ClearCache() {
}

void RTCache::Scavenge() {
  edram_store_.Scavenge();
}

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe
#endif  // RENDER_CACHE_NOT_OBSOLETE
