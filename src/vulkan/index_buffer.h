#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "../util/vk_mem_alloc.h"

namespace mb {

class IndexBuffer {
public:
  IndexBuffer(const VkDevice _logical, const VmaAllocator _allocator, std::vector<uint32_t> &_indices) 
  : logical(_logical), allocator(_allocator), indices(_indices) {
    createIndexBuffer();
  }

  ~IndexBuffer() {
    vmaDestroyBuffer(allocator, buffer, allocation);
  }

  /**
   * @brief maps index memory to the GPU
   * 
   */
  void mapMemory() {
    if (vmaCopyMemoryToAllocation(allocator, indices.data(), allocation, 0, sizeof(indices[0]) * indices.size()) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to copy vertex to GPU");
    }
  }

  uint32_t size() {return static_cast<uint32_t>(indices.size());}

  VkBuffer buffer;
private:
  const VkDevice logical;
  const VmaAllocator allocator;

  std::vector<uint32_t> indices;
  VmaAllocation allocation;

  /**
   * @brief Create an Index Buffer object
   * 
   */
  void createIndexBuffer() {
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(uint32_t) * indices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
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