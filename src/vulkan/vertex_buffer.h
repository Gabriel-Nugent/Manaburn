#pragma once

#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "../util/types.h"
#include "../util/vk_mem_alloc.h"

namespace mb {

class VertexBuffer {
public:
  VertexBuffer(const VkDevice _logical, const VmaAllocator _allocator, std::vector<Vertex> &_vertices, ) 
  : logical(_logical), allocator(_allocator), vertices(_vertices) {
  }

  ~VertexBuffer() {
    if (buffer) {
      vmaDestroyBuffer(allocator, buffer, allocation);
    }
  }

  /**
   * @brief maps vertex memory to the GPU
   * 
   */
  void mapMemory() {
    if (vmaCopyMemoryToAllocation(allocator, vertices.data(), allocation, 0, sizeof(Vertex) * vertices.size()) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to copy vertex to GPU");
    }
  }

  /**
   * @brief Create a Vertex Buffer object
   * 
   */
  void createVertexBuffer() {
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create vertex buffer");
    }
  }

  VkBuffer buffer;
private:
  const VkDevice logical;
  const VmaAllocator allocator;

  std::vector<Vertex> vertices;
  VmaAllocation allocation;
};

}