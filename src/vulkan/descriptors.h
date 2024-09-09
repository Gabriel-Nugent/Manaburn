#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

namespace mb {

class Descriptors {
public:
  Descriptors(const unsigned int FRAME_COUNT = 2) {
    createDescriptorPool(FRAME_COUNT);
  }

  ~Descriptors();

  std::vector<VkDescriptorSet> createDescriptorSets(const unsigned int FRAME_COUNT, VkDescriptorSetLayout layout);

private:
  VkDescriptorPool pool;

  void createDescriptorPool(const unsigned int FRAME_COUNT);
};

namespace DescriptorLayouts {

  VkDescriptorSetLayout createUBOLayout();

}

}