#include "descriptors.h"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace mb {

Descriptors::~Descriptors() {
  vkDestroyDescriptorPool(logical, pool, nullptr);
}

void Descriptors::createDescriptorPool(const unsigned int FRAME_COUNT) {
  VkDescriptorPoolSize poolSize {};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = static_cast<uint32_t>(FRAME_COUNT);

  VkDescriptorPoolCreateInfo poolInfo {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(FRAME_COUNT);

  if (vkCreateDescriptorPool(logical, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to create descriptor pool!");
  }
}

std::vector<VkDescriptorSet> Descriptors::createDescriptorSets(const unsigned int FRAME_COUNT, VkDescriptorSetLayout layout) {
  std::vector<VkDescriptorSetLayout> layouts(FRAME_COUNT, layout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAME_COUNT);
  allocInfo.pSetLayouts = layouts.data();

  std::vector<VkDescriptorSet> descriptorSets(FRAME_COUNT);
  if (vkAllocateDescriptorSets(logical, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  return descriptorSets;
}

namespace DescriptorLayouts {

  VkDescriptorSetLayout createUBOLayout(VkDevice logical) {
    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(logical, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create descriptor set layout");
    }

    return descriptorSetLayout;
  }

}

}