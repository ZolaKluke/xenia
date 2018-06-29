#ifndef RENDER_CACHE_NOT_OBSOLETE
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/vulkan/rt_cache.h"

namespace xe {
namespace gpu {
namespace vulkan {

using xe::ui::vulkan::CheckResult;

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
  edram_store_.Shutdown();
}

RTCache::DrawStatus RTCache::OnDraw(VkCommandBuffer command_buffer,
                                    VkFence batch_fence) {
  return DrawStatus::kNotInRenderPass;
}

void RTCache::OnFrameEnd() {
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
