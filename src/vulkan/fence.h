#pragma once

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace mb {

class Fence {
public:
  Fence(const VkDevice _logical) : logical(_logical){
    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(logical, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create fence");
    }
  }

  ~Fence() {
    vkDestroyFence(logical, fence, nullptr);
  }

  VkFence& get() {return fence;}

private:
  VkDevice logical;
  VkFence fence;

};

}