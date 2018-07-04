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
      formats[i] = DepthRenderTargetFormatToVkFormat(DepthRenderTargetFormat(
                                                     key.format));
    } else {
      if (key.is_depth) {
        assert_always();
        return false;
      }
      ColorRenderTargetFormat format = ColorRenderTargetFormat(key.format);
      format = GetBaseRTFormat(format);
      key.format = uint32_t(format);
      formats[i] = ColorRenderTargetFormatToVkFormat(format);
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
    // Need a new VkImage to get the required memory size.
    // Also will obviously need it for the view as there's no image to reuse.
    image_info.format = formats[i];
    image_info.extent.width = key.width_div_80 * 80;
    image_info.extent.height = key.height_div_16 * 16;
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
    XELOGGPU(
        "RT Cache: Need memory of size %u and alignment of %u for %ux%u image.\n",
        uint32_t(memory_requirements.size),
        uint32_t(memory_requirements.alignment), key.width_div_80 * 80,
        key.height_div_16 * 16);
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
  XELOGGPU("RT Cache: Used %u RTs.\n", used_rt_count);
  for (uint32_t i = 0; i < used_rt_count; ++i) {
    RTAllocInfo& alloc_info = alloc_infos[i];
    XELOGGPU("RT Cache: Used RT %u (%u) has page count of %u.\n", i,
             alloc_info.rt_index, alloc_info.page_count);
    uint32_t block_index;
    for (block_index = 0; block_index < 5; ++block_index) {
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
      image_info.extent.height = key.height_div_16 * 16;
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
                               rt_memory_[alloc_info.page_first / 6],
                               (alloc_info.page_first % 6) << 22);
    CheckResult(status, "vkBindImageMemory");
    VkImageView image_view = nullptr, image_view_stencil = nullptr;
    if (status == VK_SUCCESS) {
      image_view_info.image = new_images[rt_index];
      image_view_info.format = formats[rt_index];
      image_view_info.subresourceRange.aspectMask =
          key.is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
      status = vkCreateImageView(*device_, &image_view_info, nullptr,
                                 &image_view);
      CheckResult(status, "vkCreateImageView");
      if (status == VK_SUCCESS && key.is_depth) {
        image_view_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_STENCIL_BIT;
        status = vkCreateImageView(*device_, &image_view_info, nullptr,
                                   &image_view_stencil);
        CheckResult(status, "vkCreateImageView");
        if (status != VK_SUCCESS) {
          vkDestroyImageView(*device_, image_view, nullptr);
        }
      }
    }
    if (status != VK_SUCCESS) {
      for (uint32_t j = 0; j < 5; ++j) {
        VK_SAFE_DESTROY(vkDestroyImage, *device_, new_images[j], nullptr);
      }
      return false;
    }
    // TODO(Triang3l): Color and depth format names.
    device_->DbgSetObjectName(
        uint64_t(new_images[rt_index]), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
        xe::format_string("RT(%c): %u, %ux, pages [%u,%u)",
                          key.is_depth ? 'd' : 'c', key.format,
                          1 << uint32_t(key.samples), alloc_info.page_first,
                          alloc_info.page_first + alloc_info.page_count));
    // Add a new entry to the cache.
    RenderTarget* rt = new RenderTarget;
    rt->image = new_images[rt_index];
    rt->image_view = image_view;
    rt->image_view_stencil = image_view_stencil;
    rt->key.value = key.value;
    rt->page_first = alloc_info.page_first;
    rt->page_count = alloc_info.page_count;
    rt->current_usage = RenderTargetUsage::kUntransitioned;
    XELOGGPU(
        "RT Cache: Allocated RT %ux%u, format %u, samples %u, page %u, page count %u.\n",
        key.width_div_80 * 80, key.height_div_16 * 16, key.format, key.samples,
        alloc_info.page_first, alloc_info.page_count);
    rts_.insert(std::make_pair(key.value, rt));
    rts[rt_index] = rt;
    // It's not "new" anymore, so don't delete it in case of an error.
    new_images[rt_index] = nullptr;
  }

  // Return the found or created render targets.
  *rt_depth = rts[0];
  std::memcpy(rts_color, &rts[1], sizeof(rts_color));
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
      layout = VK_IMAGE_LAYOUT_GENERAL;
      if (is_depth) {
        access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
               VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      }
      access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case RenderTargetUsage::kStoreToEDRAM:
      assert_false(is_depth);
      access_mask = VK_ACCESS_SHADER_READ_BIT;
      layout = VK_IMAGE_LAYOUT_GENERAL;
      return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    default:
      assert_unhandled_case(usage);
  };
  access_mask = 0;
  layout = VK_IMAGE_LAYOUT_GENERAL;
  return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

RTCache::RenderPass* RTCache::GetRenderPass(
    const RTCache::RenderTargetKey keys_color[4],
    RTCache::RenderTargetKey key_depth) {
  // Check if there is an existing render pass with such render targets.
  // TODO(Triang3l): Use a better structure (though pass count is mostly small).
  for (auto existing_pass : passes_) {
    if (!std::memcmp(existing_pass->keys_color, keys_color, sizeof(keys_color))
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
    attachment.samples =
        VkSampleCountFlagBits(1 << uint32_t(key_depth.samples));
    // attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // TODO(Triang3l): Change loadOp to DONT_CARE when EDRAM store is added.
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachment_image_views[pass_info.attachmentCount] = rt_depth->image_view;
    width_div_80_min = std::min(key_depth.width_div_80, width_div_80_min);
    height_div_16_min = std::min(key_depth.height_div_16, height_div_16_min);
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
    attachment.samples = VkSampleCountFlagBits(1 << uint32_t(key.samples));
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_image_views[pass_info.attachmentCount] =
        rts_color[i]->image_view;
    width_div_80_min = std::min(key.width_div_80, width_div_80_min);
    height_div_16_min = std::min(key.height_div_16, height_div_16_min);
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
  std::memcpy(new_pass->rts_color, rts_color, sizeof(rts_color));
  new_pass->rt_depth = rt_depth;
  std::memcpy(new_pass->keys_color, keys_color, sizeof(keys_color));
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
  VkPipelineStageFlagBits stage_mask_src = VkPipelineStageFlagBits(0);
  VkPipelineStageFlagBits stage_mask_dst = VkPipelineStageFlagBits(0);
  VkImageMemoryBarrier image_barriers[6];
  uint32_t image_barrier_count = 0;
  if (switch_color_mask) {
    VkAccessFlags access_mask_new;
    VkImageLayout layout_new;
    stage_mask_dst =
        VkPipelineStageFlagBits(uint32_t(stage_mask_dst) | uint32_t(
        GetRenderTargetUsageParameters(false, usage, access_mask_new,
                                       layout_new)));
    for (uint32_t i = 0; i < 4; ++i) {
      if (!(switch_color_mask & (uint32_t(1) << i))) {
        continue;
      }
      RenderTarget* rt = pass->rts_color[i];
      VkImageMemoryBarrier& image_barrier = image_barriers[image_barrier_count];
      stage_mask_src =
          VkPipelineStageFlagBits(uint32_t(stage_mask_src) | uint32_t(
          GetRenderTargetUsageParameters(false, rt->current_usage,
                                         image_barrier.srcAccessMask,
                                         image_barrier.oldLayout)));
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
    VkAccessFlags access_mask_new;
    VkImageLayout layout_new;
    stage_mask_dst =
        VkPipelineStageFlagBits(uint32_t(stage_mask_dst) | uint32_t(
        GetRenderTargetUsageParameters(true, usage, access_mask_new,
                                       layout_new)));
    RenderTarget* rt = pass->rt_depth;
    VkImageMemoryBarrier& image_barrier = image_barriers[image_barrier_count];
    stage_mask_src =
        VkPipelineStageFlagBits(uint32_t(stage_mask_src) | uint32_t(
        GetRenderTargetUsageParameters(true, rt->current_usage,
                                       image_barrier.srcAccessMask,
                                       image_barrier.oldLayout)));
    rt->current_usage = usage;
    if (image_barrier.srcAccessMask != access_mask_new ||
        image_barrier.oldLayout != layout_new) {
      image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      image_barrier.pNext = nullptr;
      image_barrier.dstAccessMask = access_mask_new;
      image_barrier.newLayout = layout_new;
      image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      image_barrier.image = rt->image;
      image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      image_barrier.subresourceRange.baseMipLevel = 0;
      image_barrier.subresourceRange.levelCount = 1;
      image_barrier.subresourceRange.baseArrayLayer = 0;
      image_barrier.subresourceRange.layerCount = 1;
      ++image_barrier_count;

      VkImageMemoryBarrier& image_barrier_stencil =
          image_barriers[image_barrier_count];
      image_barrier_stencil = image_barrier;
      image_barrier_stencil.subresourceRange.aspectMask =
          VK_IMAGE_ASPECT_STENCIL_BIT;
      ++image_barrier_count;
    }
  }
  vkCmdPipelineBarrier(command_buffer, stage_mask_src, stage_mask_dst, 0, 0,
                       nullptr, 0, nullptr, image_barrier_count,
                       image_barriers);
}

void RTCache::BeginRenderPass(VkCommandBuffer command_buffer,
                              VkFence batch_fence, RTCache::RenderPass* pass) {
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

  // TODO(Triang3l): Remove clear when EDRAM store is added.
  VkClearValue clear_depth;
  begin_info.pClearValues = &clear_depth;
  if (pass->rt_depth != nullptr) {
    clear_depth.depthStencil.depth = 0.0f;
    clear_depth.depthStencil.stencil = 0;
    begin_info.clearValueCount = 1;
  } else {
    begin_info.clearValueCount = 0;
  }

  vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
  current_pass_ = pass;
}

void RTCache::EndRenderPass(VkCommandBuffer command_buffer,
                            VkFence batch_fence) {
  if (current_pass_ == nullptr) {
    return;
  }
  vkCmdEndRenderPass(command_buffer);
  current_pass_ = nullptr;
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
      SetShadowRegister(&regs.rb_color_info.value, XE_GPU_REG_RB_COLOR_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color1_info.value, XE_GPU_REG_RB_COLOR1_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color2_info.value, XE_GPU_REG_RB_COLOR2_INFO);
  dirty |=
      SetShadowRegister(&regs.rb_color3_info.value, XE_GPU_REG_RB_COLOR3_INFO);
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
  uint32_t color_info[] = {
      regs.rb_color_info.value, regs.rb_color1_info.value,
      regs.rb_color2_info.value, regs.rb_color3_info.value
  };
  MsaaSamples samples = regs.rb_surface_info.msaa_samples;
  XELOGGPU("RT Cache: %u samples in this draw.\n", samples);

  // Calculate the width of the host render target.
  uint32_t width = regs.rb_surface_info.surface_pitch;
  if (width == 0) {
    EndRenderPass(command_buffer, batch_fence);
    return DrawStatus::kDoNotDraw;
  }
  if (samples == MsaaSamples::k4X) {
    width *= 2;
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
    ColorRenderTargetFormat color_format =
        ColorRenderTargetFormat((color_info[i] >> 16) & 0xF);
    if (EDRAMStore::IsColorFormat64bpp(color_format)) {
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
    key_color.format = (color_info[i] >> 16) & 0xF;
    key_color.samples = samples;
  }
  key_depth.width_div_80 = width_div_80;
  key_depth.height_div_16 = height_div_16;
  key_depth.is_depth = 1;
  DepthRenderTargetFormat depth_format = regs.rb_depth_info.depth_format;
  key_depth.format = uint32_t(depth_format);
  key_depth.samples = samples;

  // Check if we can keep using the old pass.
  // TODO(Triang3l): Check if offsets are different when EDRAM store is added.
  if (current_pass_ != nullptr) {
    if (!std::memcmp(current_pass_->keys_color, keys_color, sizeof(keys_color))
        && current_pass_->key_depth.value == key_depth.value) {
      return DrawStatus::kDrawInSamePass;
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

void RTCache::ClearCache() {
}

void RTCache::Scavenge() {
  edram_store_.Scavenge();
}

}  // namespace vulkan
}  // namespace gpu
}  // namespace xe
#endif  // RENDER_CACHE_NOT_OBSOLETE
