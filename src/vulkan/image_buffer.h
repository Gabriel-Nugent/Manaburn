#pragma once

#include <cstdint>
#include <stdexcept>

#include "vk.h" 

namespace mb {

class ImageBuffer {
public:
  ImageBuffer(){}
  ~ImageBuffer(){clear();}

  void createImage(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo {};
    imageInfo.imageType = depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = depth;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VmaAllocationCreateInfo allocCreateInfo {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    vmaCreateImage(vk::allocator, &imageInfo, &allocCreateInfo, &image, &allocation, nullptr);
  }

  void clear() {
    if (image) {
      vmaDestroyImage(vk::allocator, image, allocation);
    }
  }

  void mapMemory(const void* data, const VkDeviceSize size, const uint32_t offset = 0) {
    if (vmaCopyMemoryToAllocation(vk::allocator, data, allocation, offset, size) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to copy image buffer to allocation");
    }
  }

private:
  VkImage image;
  VmaAllocation allocation;
};

}