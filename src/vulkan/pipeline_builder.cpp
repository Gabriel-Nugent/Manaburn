#include "pipeline_builder.h"
#include "vk.h"

#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace mb {

PipelineBuilder::PipelineBuilder() {
  clear();
}

PipelineBuilder::~PipelineBuilder() {
  clear();
}

/**
 * @brief reset all pipeline creation structs back to their empty values
 * 
 */
void PipelineBuilder::clear() {
  shaderStages.clear();
  vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  inputAssemblyInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  rasterizationInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  mutlisampleInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  depthStencilInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  colorBlendAttachment = {};
}

/**
 * @brief creates a shader module and returns it
 * 
 * @param shaderFilePath : path to the shader file
 * @return VkShaderModule : the newly create shader module
 */
VkShaderModule PipelineBuilder::createShader(std::string shaderFilePath) {
  auto shaderData = readFile(shaderFilePath);
  return createShaderModule(shaderData);
}

/**
 * @brief build the graphics pipeline with the included settings
 * 
 * @param renderPass : render pass the use in the pipeline
 * @return VkPipeline : the built pipeline
 */
VkPipeline PipelineBuilder::build(VkRenderPass renderPass) {
  // set dynamic states
  VkDynamicState dynamicState[2] = {
    VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT
  };
  VkPipelineDynamicStateCreateInfo dynamicInfo {};
  dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicInfo.dynamicStateCount = 2;
  dynamicInfo.pDynamicStates = dynamicState;

  VkPipelineViewportStateCreateInfo viewportInfo {};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportInfo.viewportCount = 1;
  viewportInfo.scissorCount = 1;

  // attach color blend attachment to state
  VkPipelineColorBlendStateCreateInfo colorBlendInfo {};
  colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendAttachment;
  colorBlendInfo.logicOpEnable = VK_FALSE;
  colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;

  VkGraphicsPipelineCreateInfo pipelineInfo {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  pipelineInfo.pRasterizationState = &rasterizationInfo;
  pipelineInfo.pViewportState = &viewportInfo;
  pipelineInfo.pMultisampleState = &mutlisampleInfo;
  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pColorBlendState = &colorBlendInfo;
  pipelineInfo.pDynamicState = &dynamicInfo;
  pipelineInfo.layout = layout;
  pipelineInfo.renderPass = renderPass;

  VkPipeline pipeline;
  if (vkCreateGraphicsPipelines(vk::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: Failed to create graphics pipeline");
  }

  return pipeline;
}

/**
 * @brief attach shaders to the pipeline
 * 
 * @param vertShader : vertex shader module
 * @param fragShader : fragment shader module
 */
void PipelineBuilder::addShaders(VkShaderModule vertShader, VkShaderModule fragShader) {
  VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShader;
  vertShaderStageInfo.pName = "main";
  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShader;
  fragShaderStageInfo.pName = "main";
  
  shaderStages.push_back(vertShaderStageInfo);
  shaderStages.push_back(fragShaderStageInfo);
}

void PipelineBuilder::setVertexInputStateEmpty() {
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;
}

/**
 * @brief set the vertex binding and attribute descriptions
 * 
 * @param vertexBindingDescriptions 
 * @param vertexAttributeDescriptions 
 */
void PipelineBuilder::setVertexInputState(
    const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
    const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
) {
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
}

/**
 * @brief sets the properties for the pipeline's input assembly
 * 
 * @param flags : input assembly flags
 * @param inputTopology : defines the primitive topology
 * @param primitiveRestartEnable : controls whether a 
 */
void PipelineBuilder::setInputAssemblyState(
    VkPrimitiveTopology inputTopology, 
    VkPipelineInputAssemblyStateCreateFlags flags, 
    VkBool32 primitiveRestartEnable
) {
  inputAssemblyInfo.topology = inputTopology;
  inputAssemblyInfo.flags = flags;
  inputAssemblyInfo.primitiveRestartEnable = primitiveRestartEnable;
}

/**
 * @brief sets the properties for the pipeline's rasterization stage
 * 
 * @param polygonMode : triangle rendering mode
 * @param cullMode : triangle facing direction
 * @param frontFace : front-facing triangle orienation to be used for culling
 * @param flags : rasterization flags
 */
void PipelineBuilder::setRasterizationState(
    VkPolygonMode polygonMode, 
    VkCullModeFlags cullMode, 
    VkFrontFace frontFace,
    VkPipelineRasterizationStateCreateFlags flags
) {
  rasterizationInfo.polygonMode = polygonMode;
  rasterizationInfo.cullMode = cullMode;
  rasterizationInfo.frontFace = frontFace;
  rasterizationInfo.flags = flags;
  rasterizationInfo.depthBiasEnable = VK_FALSE;
  rasterizationInfo.lineWidth = 1.0f; 
}

/**
 * @brief sets the properties for the pipeline's multisampling
 * 
 * @param rasterizationSamples : number of samples used in rasterization
 * @param flags : flags for multisampling
 */
void PipelineBuilder::setMultisampleState(
    VkSampleCountFlagBits rasterizationSamples,
    VkPipelineMultisampleStateCreateFlags flags
) {
  mutlisampleInfo.rasterizationSamples = rasterizationSamples;
  mutlisampleInfo.flags = flags;
}

void PipelineBuilder::setMultisamplingNone() {
  mutlisampleInfo.sampleShadingEnable = VK_FALSE;
  // multisampling defaulted to no multisampling (1 sample per pixel)
  mutlisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  mutlisampleInfo.minSampleShading = 1.0f;
  mutlisampleInfo.pSampleMask = nullptr;
  // no alpha to coverage either
  mutlisampleInfo.alphaToCoverageEnable = VK_FALSE;
  mutlisampleInfo.alphaToOneEnable = VK_FALSE;
}

/**
 * @brief sets the properties for the pipeline's 
 * 
 * @param depthTestEnable : determines whether depth testing is enabled
 * @param depthWriteEnable : determines whehter depth writing is enabled
 * @param depthCompareOp : operator to use during the depth comparison stage
 */
void PipelineBuilder::setDepthStencilState(
    VkBool32 depthTestEnable, 
    VkBool32 depthWriteEnable,
    VkCompareOp depthCompareOp
) {
  depthStencilInfo.depthBoundsTestEnable = depthWriteEnable;
  depthStencilInfo.depthWriteEnable = depthWriteEnable;
  depthStencilInfo.depthCompareOp = depthCompareOp;
  depthStencilInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
}

void PipelineBuilder::disableDepthtest() {
  depthStencilInfo.depthTestEnable = VK_FALSE;
  depthStencilInfo.depthWriteEnable = VK_FALSE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_NEVER;
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilInfo.stencilTestEnable = VK_FALSE;
  depthStencilInfo.front = {};
  depthStencilInfo.back = {};
  depthStencilInfo.minDepthBounds = 0.f;
  depthStencilInfo.maxDepthBounds = 1.f;
}

/**
 * @brief set the pipeline layout
 * 
 * @param pipelineLayout : the layout to attach to the pipeline
 */
void PipelineBuilder::setPipelineLayout(VkPipelineLayout pipelineLayout) {
  layout = pipelineLayout;
}

/**
 * @brief disables color blending in the graphics pipeline
 * 
 */
void PipelineBuilder::disableColorBlending() {
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
}

/**
 * @brief enables additive blending in the graphics pipeline
 * 
 */
void PipelineBuilder::enableBlendingAdditive() {
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

/**
 * @brief enables alpha blending in the graphics pipeline
 * 
 */
void PipelineBuilder::enableAlphaBlend() {
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

/**
 * @brief stores the binary data of a provided file
 * 
 * @param filename : name of the file in string format
 * @return std::vector<char> : binary data buffer
 */
std::vector<char> PipelineBuilder::readFile(const std::string& filename) {
  // start reading at the end of the file
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("[ERROR]: failed to open file: " + filename);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

/**
 * @brief Creates a Shader Module object 
 *  
 * @param code : data buffer of shader file
 * @return VkShaderModule 
 */
VkShaderModule PipelineBuilder::createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo moduleInfo{};
  moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleInfo.codeSize = code.size();
  moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(vk::device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: Failed to create shader module");
  }

  return shaderModule;
}

}