#pragma once

#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "../util/types.h"
#include "../util/vk_mem_alloc.h"

namespace mb {

class UniformBuffer {
public:
  UniformBuffer(const VkDevice _logical, const VmaAllocator _allocator) 
  : logical(_logical), allocator(_allocator) {
    createUniformBuffer();
  }

  ~UniformBuffer() {
    vmaDestroyBuffer(allocator, buffer, allocation);
  }

  /**
   * @brief maps memory to the GPU
   * 
   */
  void mapMemory(UniformBufferObject &uniformData) {
    if (vmaCopyMemoryToAllocation(allocator, &uniformData, allocation, 0, sizeof(UniformBufferObject)) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to copy vertex to GPU");
    }
  }

  VkBuffer buffer;
private:
  const VkDevice logical;
  const VmaAllocator allocator;

  VmaAllocation allocation;

  /**
   * @brief Create a Uniform Buffer object
   * 
   */
  void createUniformBuffer() {
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(UniformBufferObject);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create vertex buffer");
    }
  }
};

}