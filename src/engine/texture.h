#pragma once

#include <memory>
#include <string>

#include <vulkan/vulkan_core.h>

#include "../vulkan/image_buffer.h"

namespace mb {

class Texture {
public:
  Texture(const std::string filePath) {
    createTextureImage(filePath);
  }

  std::unique_ptr<ImageBuffer> image;
   
private:

  void createTextureImage(const std::string filePath);
};

}