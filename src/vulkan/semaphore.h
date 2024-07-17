#pragma once

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace mb {

class Semaphore {
public:
  Semaphore(const VkDevice _logical) : logical(_logical){
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(logical, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create semaphore");
    }
  }

  ~Semaphore() {
    vkDestroySemaphore(logical, semaphore, nullptr);
  }

  VkSemaphore& get() {return semaphore;}

private:
  VkDevice logical;
  VkSemaphore semaphore;
};

}