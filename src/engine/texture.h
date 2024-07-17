#pragma once

#include <memory>
#include <string>

#include "../vulkan/image_buffer.h"

namespace mb {

class Texture {
public:
  Texture(VkDevice _logical, VmaAllocator _allocator, const std::string filePath)
  : logical(_logical), allocator(_allocator) {
    createTextureImage(filePath);
  }

  std::unique_ptr<ImageBuffer> image;
   
private:
  VkDevice logical;
  VmaAllocator allocator;

  void createTextureImage(const std::string filePath);
};

}