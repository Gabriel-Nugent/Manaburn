#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan_core.h>

namespace mb {

class PipelineBuilder {
public:
  PipelineBuilder();
  ~PipelineBuilder();

  void clear();
  VkPipeline build(VkRenderPass renderPass);

  VkShaderModule static createShader(std::string shaderFilePath);
  void addShaders(VkShaderModule vertShader, VkShaderModule fragShader);
  void setVertexInputStateEmpty();
  void setVertexInputState(
      const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
      const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
  );
  void setInputAssemblyState(
      VkPrimitiveTopology inputTopology,
      VkPipelineInputAssemblyStateCreateFlags flags = 0, 
      VkBool32 primitiveRestartEnable = VK_FALSE
  );
  void setRasterizationState(
      VkPolygonMode polygonMode, 
      VkCullModeFlags cullMode, 
      VkFrontFace frontFace,
      VkPipelineRasterizationStateCreateFlags flags = 0
  );
  void setMultisampleState(
      VkSampleCountFlagBits rasterizationSamples,
      VkPipelineMultisampleStateCreateFlags flags = 0
  );
  void setMultisamplingNone();
  void setDepthStencilState(
      VkBool32 depthTestEnable,
      VkBool32 depthWriteEnable,
      VkCompareOp depthCompareOp
  );
  void disableDepthtest();
  void setPipelineLayout(VkPipelineLayout pipelineLayout);
  void disableColorBlending();
  void enableBlendingAdditive();
  void enableAlphaBlend();

private:
  VkPipelineLayout layout;

  // structs for pipeline creation
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo mutlisampleInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;

  // helper functions
  std::vector<char> static readFile(const std::string& filename);
  VkShaderModule static createShaderModule(const std::vector<char>& code);
};

}