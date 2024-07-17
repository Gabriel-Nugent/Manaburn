#include "texture.h"
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../util/stb_image.h"

namespace mb {

void Texture::createTextureImage(const std::string filePath) {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("[ERROR]: failed to load texture image at: " + filePath);
  }

  const VkDeviceSize imageSize = texWidth * texHeight * 4;

  image = std::make_unique<ImageBuffer>(logical, allocator);
  image->createImage(texWidth, texHeight, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  image->mapMemory(pixels, imageSize);

  stbi_image_free(pixels);
}

}