#include "engine.h"

#include "../util/types.h"

#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_video.h>

#include <chrono>
#include <cstdint>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <stdexcept>
#include <synchapi.h>
#include <thread>
#include <utility>
#include <windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

namespace mb {

/**
 * @brief primes the engine
 * 
 */
void Engine::init() {
  vk = std::make_unique<VKInterface>();
  vk->init();
  // initialize frames
  initPipelines();
  initFrames();
  initMeshes();
}

/**
 * @brief main game loop for polling events and
 *        drawing graphics
 */
void Engine::run() {
  SDL_Event event;
  bool shouldQuit = false;

  while (!shouldQuit) {
    // handle window events
    while (SDL_PollEvent(&event) != 0) {
      switch(event.type) {
        case SDL_QUIT:
          shouldQuit = true;
          break;
        case SDL_WINDOWEVENT:
          switch(event.window.event) {
            case SDL_WINDOWEVENT_MINIMIZED:
              stop_rendering = true;
              break;
            case SDL_WINDOWEVENT_RESTORED:
              stop_rendering = false;
              break;
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
              framebufferResized = true;
              break;
            default:
              break;
          }
        default:
          break;
      }
    }

    // halt drawing on minimization
    if (stop_rendering) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (framebufferResized) {
      vk->swapchain->recreate();
      framebufferResized = false;
    }

    // draw functions
    drawFrame();
  }

  vkDeviceWaitIdle(vk->device->logical);
  cleanup();
}

/**
 * @brief tear down engine and free memory
 * 
 */
void Engine::cleanup() {
  for (auto& [name, mesh] : meshes) {
    mesh.clear();
  }
  for (int i = 0; i < FRAME_COUNT; i++) {
    cmdBuffers[i].reset();
    imageAvailableSemaphores[i].reset();
    renderFinishedSemaphores[i].reset();
    inFlightFences[i].reset();
    uniformBuffers[i].reset();
  }
  for (const auto& [name, pipeline] : pipelines) {
    vkDestroyPipeline(vk->device->logical, pipeline, nullptr);
  }
  for (const auto& [name, layout] : descriptorLayouts) {
    vkDestroyDescriptorSetLayout(vk->device->logical, layout, nullptr);
  }
  for (const auto& [name, layout] : pipelineLayouts) {
    vkDestroyPipelineLayout(vk->device->logical, layout, nullptr);
  }
  vk.reset();
}

/**
 * @brief create layouts and pipelines
 * 
 */
void Engine::initPipelines() {
  descriptorLayouts["ubo-object"] = DescriptorLayouts::createUBOLayout(vk->device->logical);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorLayouts["ubo-object"];

  VkPipelineLayout layout;
  if (vkCreatePipelineLayout(vk->device->logical, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to create pipeline layout");
  }
  pipelineLayouts["empty-layout"] = layout;

  auto vertShader = PipelineBuilder::createShader("shaders/basic_shader.vert.spv", vk->device->logical);
  auto fragShader = PipelineBuilder::createShader("shaders/basic_shader.frag.spv", vk->device->logical);

  auto bindingDescriptions = Vertex::getBindingDescriptions();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  PipelineBuilder builder(vk->device->logical);
  builder.setPipelineLayout(layout);
  builder.addShaders(vertShader,fragShader);
  builder.setVertexInputState(bindingDescriptions, attributeDescriptions);
  builder.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  builder.setRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
  builder.setMultisampleState(VK_SAMPLE_COUNT_1_BIT);
  builder.disableColorBlending();
  auto pipeline = builder.build(vk->swapchain->renderPass);
  pipelines["basic-pipeline"] = pipeline;

  vkDestroyShaderModule(vk->device->logical, vertShader, nullptr);
  vkDestroyShaderModule(vk->device->logical, fragShader, nullptr);
}

/**
 * @brief initialize command buffers and sync structures for frames
 * 
 */
void Engine::initFrames() {
  for (int i = 0; i < FRAME_COUNT; i++) {
    cmdBuffers.push_back(std::make_unique<Command>(vk->device));
    imageAvailableSemaphores.push_back(std::make_unique<Semaphore>(vk->device->logical));
    renderFinishedSemaphores.push_back(std::make_unique<Semaphore>(vk->device->logical));
    inFlightFences.push_back(std::make_unique<Fence>(vk->device->logical));
    uniformBuffers.push_back(std::make_unique<UniformBuffer>(vk->device->logical, vk->allocator));
  }
  // immediate submit
  uploadContext.uploadFence = std::make_unique<Fence>(vk->device->logical);
  uploadContext.cmd = std::make_unique<Command>(vk->device);

  // initalize descriptors
  descriptorSets = vk->descriptors->createDescriptorSets(FRAME_COUNT, descriptorLayouts["ubo-object"]);

  for (size_t i = 0; i < FRAME_COUNT; i++) {
    VkDescriptorBufferInfo bufferInfo {};
    bufferInfo.buffer = uniformBuffers[i]->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;

    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;

    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(vk->device->logical, 1, &descriptorWrite, 0, nullptr);
  }
}

/**
 * @brief init meshes
 * 
 */
void Engine::initMeshes() {
  const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
  };

  std::vector<uint32_t> indices ={
    0, 1, 2, 2, 3, 0
  };

  meshes.emplace("rectangle_mesh", Mesh(vk->device->logical,vk->allocator, vertices));
  meshes["rectangle_mesh"].setIndexBuffer(indices);
}

/**
 * @brief main drawing functions go here
 * 
 */
void Engine::drawFrame() {
  // wait for gpu to render last frame
  vkWaitForFences(vk->device->logical, 1, &inFlightFences[currentFrame]->get(), true, 1000000000);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(vk->device->logical, vk->swapchain->get(), UINT64_MAX, imageAvailableSemaphores[currentFrame]->get(), VK_NULL_HANDLE, &imageIndex);
  // check for out of date swapchain
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    framebufferResized = true;
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("[ERROR]: failed to acquire swapchain image");
  }
 
  vkResetFences(vk->device->logical, 1, &inFlightFences[currentFrame]->get());

  // reset command buffer to begin recording again
  vkResetCommandBuffer(cmdBuffers[currentFrame]->buffer, 0);

  // update descriptor sets
  updateUniformBuffer(currentFrame);

  recordCommandBuffer(cmdBuffers[currentFrame]->buffer, imageIndex);

  result = submitFrame(currentFrame, imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    framebufferResized = true;
  } else if (result != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to present swapchain image");
  }

  currentFrame = (currentFrame + 1) % FRAME_COUNT;
}

void Engine::updateUniformBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), vk->swapchain->swapchainExtent.height / (float) vk->swapchain->swapchainExtent.height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  uniformBuffers[currentImage]->mapMemory(ubo);
}

/**
 * @brief prepare command buffer for draw commands
 * 
 * @param buffer : command buffer to begin rendering commands to
 * @param imageIndex : image of the swapchain image to write to
 */
void Engine::recordCommandBuffer(const VkCommandBuffer buffer, const uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;
  
  if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to begin recording command buffer");
  }

  VkRenderPassBeginInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vk->swapchain->renderPass;
  renderPassInfo.framebuffer = vk->swapchain->framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0,0};
  renderPassInfo.renderArea.extent = vk->swapchain->swapchainExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines["basic-pipeline"]);

  VkViewport viewport {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(vk->swapchain->swapchainExtent.width);
  viewport.height = static_cast<float>(vk->swapchain->swapchainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = vk->swapchain->swapchainExtent;
  vkCmdSetScissor(buffer, 0, 1, &scissor);

  VkBuffer vertexBuffers[] = {meshes["rectangle_mesh"].getVertexBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(buffer, meshes["rectangle_mesh"].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts["empty-layout"], 0, 1, &descriptorSets[currentFrame], 0, nullptr);

  vkCmdDrawIndexed(buffer, meshes["rectangle_mesh"].indexBufferSize(), 1, 0, 0, 0);

  vkCmdEndRenderPass(buffer);

  if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to record command buffer");
  }
}

VkResult Engine::submitFrame(const uint32_t currentFrame, const uint32_t imageIndex) {
  VkSubmitInfo submitInfo {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]->get()};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffers[currentFrame]->buffer;

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]->get()};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(vk->device->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]->get()) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: Failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {vk->swapchain->get()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &imageIndex;

  return vkQueuePresentKHR(vk->device->presentQueue, &presentInfo);
}

void Engine::immediateSubmit(std::function<void(VkCommandBuffer)>&& function) {
  VkCommandBuffer cmd = uploadContext.cmd->buffer;

  VkCommandBufferBeginInfo cmdInfo {};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdInfo.pNext = nullptr;
	cmdInfo.pInheritanceInfo = nullptr;
	cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(cmd, &cmdInfo) != VK_SUCCESS) {
    throw std::runtime_error("{ERROR]: failed to begin command buffer");
  }

  function(cmd);

  if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
    throw std::runtime_error("{ERROR]: failed to end command buffer");
  }

  VkSubmitInfo submitInfo {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

  if (vkQueueSubmit(vk->device->graphicsQueue, 1, &submitInfo, uploadContext.uploadFence->get()) != VK_SUCCESS) {
    throw std::runtime_error("{ERROR]: submit command buffer");
  }

  vkWaitForFences(vk->device->logical, 1, &uploadContext.uploadFence->get(), true, 9999999999);
	vkResetFences(vk->device->logical, 1, &uploadContext.uploadFence->get());

  vkResetCommandPool(vk->device->logical, uploadContext.cmd->pool, 0);
}

void Engine::uploadMesh(Mesh& mesh) {
  const size_t bufferSize = mesh.size() * sizeof(Vertex);

  VkBufferCreateInfo stagingBufferInfo {};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.pNext = nullptr;
  stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo vmaAllocInfo {};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;

  if (vmaCreateBuffer(vk->allocator, &stagingBufferInfo, &vmaAllocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to create staging buffer");
  }

  if (vmaCopyMemoryToAllocation(vk->allocator, &mesh.vertices, stagingAllocation, 0, bufferSize) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to copy memory to allocation");
  }

  
}

}