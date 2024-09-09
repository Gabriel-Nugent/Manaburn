#pragma once

#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "vk.h"

namespace mb {

class Buffer {
public:
  Buffer() {}

  ~Buffer() {clear();}

  /**
   * @brief maps vertex memory to the GPU
   * 
   */
  void copyMemoryToAllocation(void* data, VkDeviceSize bufferSize) {
    if (vmaCopyMemoryToAllocation(vk::allocator, data, allocation, 0, bufferSize) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to copy vertex to GPU");
    }
  }

  void clear() {
      if (buffer) {
      vmaDestroyBuffer(vk::allocator, buffer, allocation);
    }
  }


  /**
   * @brief Create a Vertex Buffer object
   * 
   */
  void allocateBuffer(VkBufferUsageFlags usage, VmaMemoryUsage memUsage, VmaAllocationCreateFlags memFlags, VkDeviceSize bufferSize) {
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = memUsage;
    allocInfo.flags = memFlags;

    if (vmaCreateBuffer(vk::allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: Failed to create buffer");
    }
  }

  VkBuffer buffer;
private:
  VmaAllocation allocation;
};

}