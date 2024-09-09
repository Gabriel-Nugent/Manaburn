#pragma once

#include <stdexcept>

#include "vk.h"

namespace mb {

class Semaphore {
public:
  Semaphore() {
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(vk::device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create semaphore");
    }
  }

  ~Semaphore() {
    clear();
  }

  void clear() {
    if (semaphore) {
      vkDestroySemaphore(vk::device, semaphore, nullptr);
    }
  }

  VkSemaphore& get() {return semaphore;}

private:
  VkSemaphore semaphore;
};

}