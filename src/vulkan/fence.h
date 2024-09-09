#pragma once

#include <stdexcept>

#include "vk.h"

namespace mb {

class Fence {
public:
  Fence() {
    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(vk::device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create fence");
    }
  }

  ~Fence() {
    clear();
  }

  void clear() {
    if (fence) {
      vkDestroyFence(vk::device, fence, nullptr);
    }
  }

  VkFence& get() {return fence;}

private:
  VkFence fence;

};

}