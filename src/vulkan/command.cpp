#include "command.h"
#include "vk.h"

#include <stdexcept>


namespace mb {
  
  Command::~Command() {
    vkDestroyCommandPool(vk::device, pool, nullptr);
  }
  
  /**
   * @brief create a command pool and allocate a main command buffer from it
   * 
   */
  void Command::init() {
    createCommandPool();
    allocateCommandBuffer();
  }

  /**
   * @brief create a command pool from which to allocate commands from
   * 
   */
  void Command::createCommandPool() {
    VkCommandPoolCreateInfo commandPoolInfo {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = vk::queueIndices.graphicsFamily.value();

    if (vkCreateCommandPool(vk::device, &commandPoolInfo, nullptr, &pool) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: Failed to create command pool");
    }
  }

  /**
   * @brief allocate a main command buffer from the command pool
   * 
   */
  void Command::allocateCommandBuffer() {
    VkCommandBufferAllocateInfo commandBufferInfo {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandPool = pool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(vk::device, &commandBufferInfo, &buffer) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: Failed to allocate command buffer");
    }
  }

}