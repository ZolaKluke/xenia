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
    ColorRenderTargetFormat format) {
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
    DepthRenderTargetFormat format) {
  switch (format) {
    case DepthRenderTargetFormat::kD24S8:
      return VK_FORMAT_D24_UNORM_S8_UINT;
    case DepthRenderTargetFormat::kD24FS8:
      // Vulkan doesn't support 24-bit floats, so just promote it to 32-bit
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

  return VK_SUCCESS;
}

void RTCache::Shutdown() {
  // TODO(Triang3l): Wait for idle.

  for (auto rt : rts_) {
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

bool RTCache::AllocateRenderTargets(
    const RTCache::RenderTargetKey keys_color[4],
    RTCache::RenderTargetKey key_depth, RTCache::RenderTarget* rts_color[4],
    RTCache::RenderTarget** rt_depth) {
  // Since the logic is the same for color and depth, combine them.
  // Prefer placing in a more stable position (closer to the first page) so it
  // has less different views.
  RenderTargetKey keys[5];
  keys[0].value = key_depth.value;
  std::memcpy(&keys[1], keys_color, sizeof(keys_color));

  // Validate attachments and get their Vulkan formats.
  VkFormat formats[5];
  for (uint32_t i = 0; i < 5; ++i) {
    RenderTargetKey& key = keys[i];
    if (key.value == 0) {
      continue;
    }
    if (key.width_div_80 == 0 || key.height_div_16 == 0) {
      assert_always();
      return false;
    }
    if (i == 0) {
      if (!key.is_depth) {
        assert_always();
        return false;
      }
      formats[i] = DepthRenderTargetFormatToVkFormat(key.depth_format);
    } else {
      if (key.is_depth) {
        assert_always();
        return false;
      }
      key.color_format = GetBaseRTFormat(key.color_format);
      formats[i] = ColorRenderTargetFormatToVkFormat(key.color_format);
    }
    if (formats[i] == VK_FORMAT_UNDEFINED) {
      assert_always();
      return false;
    }
  }

  VkResult status;

  // Prepare for image creation.
  VkImageCreateInfo image_info;
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.pNext = nullptr;
  image_info.flags = 0;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.queueFamilyIndexCount = 0;
  image_info.pQueueFamilyIndices = nullptr;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  const VkImageUsageFlags image_usage_color =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  const VkImageUsageFlags image_usage_depth =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  // Find page count for each render target.
  struct RTAllocInfo {
    uint32_t rt_index;
    uint32_t page_first;
    uint32_t page_count;
  };
  RTAllocInfo alloc_infos[5];
  uint32_t used_rt_count = 0;
  VkImage new_images[5] = {};
  for (uint32_t i = 0; i < 5; ++i) {
    RenderTargetKey key(keys[i]);
    if (key.value == 0) {
      continue;
    }
    RTAllocInfo& alloc_info = alloc_infos[user_rt_count];
    alloc_info.rt_index = i;
    auto found_rt_iter = rts_.find(key.value);
    if (found_rt_iter != rts_.end()) {
      // There is a render target with the requested parameters already.
      // Reusing its page count. It may be aliased, but this doesn't matter.
      alloc_info.page_count = (*found_rt_iter)->page_count;
      ++used_rt_count;
      continue;
    }
    // Need a new VkImage to get the required memory size.
    // Also will obviously need it for the view as there's no image to reuse.
    image_info.format = formats[i];
    image_info.extent.width = key.width_div_80 * 80;
    image_info.extent.height = key.width_div_16 * 16;
    image_info.samples = VkSampleCountFlagBits(1 << uint32_t(key.samples));
    image_info.usage = key.is_depth ? image_usage_depth : image_usage_color;
    status = vkCreateImage(*device_, &image_info, nullptr, &new_images[i]);
    CheckResult(status, "vkCreateImage");
    if (status != VK_SUCCESS) {
      for (uint32_t j = 0; j < i; ++j) {
        VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
      }
      return false;
    }
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(*device_, new_images[i], &memory_requirements);
    assert_always(memory_requirements.alignment <= (1 << 22));
    if (memory_requirements.size > (6 << 22)) {
      // Can't fit the image in a whole 24 MB block.
      assert_always();
      for (uint32_t j = 0; j < i; ++j) {
        VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
      }
      return false;
    }
    alloc_info.page_count =
        xe::round_up(uint32_t(memory_requirements.size), 1u << 22) >> 22;
    ++used_rt_count;
  }
  if (used_rt_count == 0) {
    std::memset(rts_color, 0, sizeof(rts_color));
    *rt_depth = nullptr;
    return true;
  }

  // Try to pack the framebuffers tightly.
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
    const RTAllocInfo& alloc_info = alloc_infos[i];
    for (uint32_t block_index = 0; block_index < 5; ++block_index) {
      if (pages_allocated[block_index] + alloc_info.page_count <= 6) {
        break;
      }
    }
    if (block_index >= 5) {
      // Couldn't find a block - this must not happen as there are 5.
      assert_always();
      for (uint32_t j = 0; j < 5; ++j) {
        VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
      }
      return false;
    }
    alloc_info.page_first = block_index * 6 + pages_allocated[block_index];
    pages_allocated[block_index] += alloc_info.page_count;
  }

  // Create the needed memory blocks if they don't exist yet.
  VkMemoryRequirements block_memory_requirements;
  block_memory_requirements.size = 6 << 22;
  block_memory_requirements.alignment = 1 << 22;
  // Let's assume we can just use any device-local memory for render targets.
  block_memory_requirements.memoryTypeBits = UINT32_MAX;
  for (uint32_t i = 0; i < 5; ++i) {
    if (pages_allocated[i] != 0 && rt_memory_[i] == nullptr) {
      rt_memory_[i] =
          device_->AllocateMemory(block_memory_requirements,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      if (rt_memory_[i] == nullptr) {
        assert_always();
        for (uint32_t j = 0; j < 5; ++j) {
          VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
        }
        return false;
      }
    }
  }

  // Find or create the needed render targets.
  RenderTarget* rts[5] = {};
  VkImageViewCreateInfo image_view_info;
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.pNext = nullptr;
  image_view_info.flags = 0;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
  image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
  image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
  image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
  image_view_info.subresourceRange.baseMipLevel = 0;
  image_view_info.subresourceRange.levelCount = 1;
  image_view_info.subresourceRange.baseArrayLayer = 0;
  image_view_info.subresourceRange.layerCount = 1;
  for (uint32_t i = 0; i < used_rt_count; ++i) {
    const RTAllocInfo& alloc_info = alloc_infos[i];
    uint32_t rt_index = alloc_info.rt_index;
    RenderTargetKey key(keys[rt_index]);
    auto found_rts = rts_.equal_range(key.value);
    for (auto iter = found_rts.first; iter != found_rts.second; ++iter) {
      RenderTarget* found_rt = iter->second;
      if (found_rt->page_first == alloc_info.page_first) {
        // Found an existing render target in a matching page range, use it.
        rts[rt_index] = found_rt;
        break;
      }
    }
    if (rts[rt_index] != nullptr) {
      continue;
    }
    // Create a new render target image if needed.
    if (new_images[rt_index] == nullptr) {
      image_info.format = formats[rt_index];
      image_info.extent.width = key.width_div_80 * 80;
      image_info.extent.height = key.width_div_16 * 16;
      image_info.samples = VkSampleCountFlagBits(1 << uint32_t(key.samples));
      image_info.usage = key.is_depth ? image_usage_depth : image_usage_color;
      status = vkCreateImage(*device_, &image_info, nullptr, &new_images[rt_index]);
      CheckResult(status, "vkCreateImage");
      if (status != VK_SUCCESS) {
        for (uint32_t j = 0; j < 5; ++j) {
          VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
        }
        return false;
      }
    }
    // Bind memory to it and create a view.
    status = vkBindImageMemory(*device_, new_images[rt_index],
                               rt_memory_[alloc_info->page_first / 6],
                               (alloc_info->page_first % 6) << 22);
    CheckResult(status, "vkBindImageMemory");
    VkImageView image_view = nullptr;
    if (status == VK_SUCCESS) {
      image_view_info.image = new_images[rt_index];
      image_view_info.format = formats[rt_index];
      if (key.is_depth) {
        image_view_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      } else {
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      }
      status = vkCreateImageView(*device_, &image_view_info, nullptr,
                                 &image_view);
      CheckResult(status, "vkCreateImageView");
    }
    if (status != VK_SUCCESS) {
      for (uint32_t j = 0; j < 5; ++j) {
        VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
      }
      return false;
    }
    // Add a new entry to the cache.
    RenderTarget* rt = new RenderTarget;
    rt->image = new_images[rt_index];
    rt->image_view = image_view;
    rt->key.value = key.value;
    rt->page_first = alloc_info.page_first;
    rt->page_count = alloc_info.page_count;
    rts_.insert(std::make_pair<uint32_t, RenderTarget*>(key.value, rt));
    rts[rt_index] = rt;
    // It's not "new" anymore, so don't delete it in case of an error.
    new_images[rt_index] = nullptr;
  }

  // Return the found or created render targets.
  *rt_depth = rts[0];
  std::memcpy(rts_color, &rts[1], sizeof(rts_color));
  return true;
}

RTCache::DrawStatus RTCache::OnDraw(VkCommandBuffer command_buffer,
                                    VkFence batch_fence) {
  return DrawStatus::kNotInRenderPass;
}

void RTCache::OnFrameEnd(VkCommandBuffer command_buffer, VkFence batch_fence) {
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
