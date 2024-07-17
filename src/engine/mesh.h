#pragma once

#include "../util/types.h"
#include "../util/vk_mem_alloc.h"

#include "../vulkan/vertex_buffer.h"
#include "../vulkan/index_buffer.h"

#include <memory>
#include <vector>

namespace mb {

class Mesh {
public:
  Mesh(const VkDevice _logical,const VmaAllocator _allocator,std::vector<Vertex> _vertices) 
  : logical(_logical), allocator(_allocator), vertices(_vertices) {
    vertexBuffer = std::make_unique<VertexBuffer>(logical, allocator, vertices);
  }
  
  /**
   * @brief transfers mesh vertex data to the GPU
   * 
   */
  void upload() {
    vertexBuffer->mapMemory();
    indexBuffer->mapMemory();
  }
  
  void clear() {
    vertexBuffer.reset();
    indexBuffer.reset();
  }

  VkBuffer getVertexBuffer() {return vertexBuffer->buffer;}
  VkBuffer getIndexBuffer() {return indexBuffer->buffer;}

  void setIndexBuffer(std::vector<uint32_t> &indices) {indexBuffer = std::make_unique<IndexBuffer>(logical, allocator, indices);}

  uint32_t size() {return static_cast<uint32_t>(vertices.size());}
  uint32_t indexBufferSize() {return indexBuffer->size();}

  std::vector<Vertex> vertices;
private:
  const VkDevice logical;
  const VmaAllocator allocator;

  std::unique_ptr<VertexBuffer> vertexBuffer;
  std::unique_ptr<IndexBuffer> indexBuffer = nullptr;
};

}