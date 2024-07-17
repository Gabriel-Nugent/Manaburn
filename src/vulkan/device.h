#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace mb {

/**
 * @brief simple struct for holding queue indices
 * 
 */
struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

/**
 * @brief Vulkan device extensions required by the app
 * 
 */
const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/**
 * @brief wrapper for Vulkan's GPU handler
 * 
 */
class Device {
public:
  // vulkan handlers
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkDevice logical;
  QueueFamilyIndices queueIndices;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  
  Device(const VkInstance _instance, const VkSurfaceKHR _surface);
  ~Device();

private:
  // vulkan handlers initialized in other classes
  const VkInstance instance;
  const VkSurfaceKHR surface;

  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  void getQueueFamilies();
  void createLogicalDevice();
  std::vector<const char*> getRequiredExtensions();
  void getQueues();
};

}