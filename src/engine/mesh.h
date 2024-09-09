#pragma once

#include "../util/types.h"
#include "../util/vk_mem_alloc.h"

#include "../vulkan/buffer.h"

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

namespace mb {

class Mesh {
public:
  Mesh() {}
  Mesh(std::vector<Vertex>& vertices) : vertices(vertices) {} 

  uint32_t size() {return vertices.size() * sizeof(Vertex);}
  uint32_t vertexCount() {return vertices.size();}
  void copyToAllocation() {vertexBuffer.copyMemoryToAllocation(vertices.data(), size());}

  Buffer vertexBuffer;

private:
  std::vector<Vertex> vertices;
};

}