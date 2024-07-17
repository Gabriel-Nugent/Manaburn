#pragma once

#include "../vulkan/vk_interface.h"
#include "../vulkan/command.h"
#include "../vulkan/fence.h"
#include "../vulkan/semaphore.h"
#include "../vulkan/uniform_buffer.h"
#include "../vulkan/descriptors.h"

#include "mesh.h"
#include "texture.h"

#include <SDL_stdinc.h>
#include <functional>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mb {

constexpr unsigned int FRAME_COUNT = 2;

struct UploadContext {
  std::unique_ptr<Fence> uploadFence;
  std::unique_ptr<Command> cmd;
};

/**
 * @brief Main engine for controlling all processes
 * 
 */
class Engine {
public:
  Engine(){};

  Engine (const Engine&) = delete;
  Engine& operator= (const Engine&) = delete;

  void init();
  void run();
  void cleanup();

private:
  std::unique_ptr<VKInterface> vk;

  std::unordered_map<std::string, VkDescriptorSetLayout> descriptorLayouts;
  std::unordered_map<std::string, VkPipelineLayout> pipelineLayouts;
  std::unordered_map<std::string, VkPipeline> pipelines;
  std::unordered_map<std::string, Mesh> meshes;
  std::unordered_map<std::string, std::unique_ptr<Texture>>  texures;

  // engine states
  uint32_t currentFrame = 0;
  bool framebufferResized = false;
  bool stop_rendering = false;

  // per frame objects
  std::vector<std::unique_ptr<Command>> cmdBuffers;
  std::vector<std::unique_ptr<Semaphore>> imageAvailableSemaphores;
  std::vector<std::unique_ptr<Semaphore>> renderFinishedSemaphores;
  std::vector<std::unique_ptr<Fence>> inFlightFences;
  std::vector<std::unique_ptr<UniformBuffer>> uniformBuffers;
  std::vector<VkDescriptorSet> descriptorSets;

  // for imediate submit
  UploadContext uploadContext;

  void initPipelines();
  void initFrames();
  void initMeshes();

  void drawFrame();
  void updateUniformBuffer(uint32_t currentImage);
  void recordCommandBuffer(const VkCommandBuffer buffer, const uint32_t imageIndex);
  VkResult submitFrame(const uint32_t currentFrame, const uint32_t imageIndex);
  void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
  void uploadMesh(Mesh& mesh);
};

}