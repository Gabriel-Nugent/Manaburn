#include "device.h"

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <set>

#include <vulkan/vulkan_core.h>

namespace mb {

Device::Device(const VkInstance _instance, const VkSurfaceKHR _surface) 
: instance(_instance),  surface(_surface) {
  pickPhysicalDevice();
  getQueueFamilies();
  createLogicalDevice();
  getQueues();
}

Device::~Device() {
  vkDestroyDevice(logical, nullptr);
}

/**
 * @brief search for GPUs and select the best one for the application
 * 
 */
void Device::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("[ERROR]: failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
      physical = device;
      break;
    }
  }

  if (physical == VK_NULL_HANDLE) {
    throw std::runtime_error("[ERROR]: failed to find suitable GPU");
  }
}

/**
 * @brief checks a GPU for a set of application requirements
 * 
 * @param device : GPU to check against requirements
 * @return true : if device meets requirements
 * @return false : if device does not meet requirements
 */
bool Device::isDeviceSuitable(VkPhysicalDevice device) {
  return true;
}

/**
 * @brief finds the indices of the queue families for the selected GPU
 * 
 */
void Device::getQueueFamilies() {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    // check for graphics support
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    // check for present support
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }
    // check if all queue families have been filled
    if (indices.isComplete()) {
      break;
    }

    i++;
  }

  queueIndices = indices;
}

/**
 * @brief creates a logical device of which to interact with Vulkan
 * 
 */
void Device::createLogicalDevice() {
  // info for queues
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {
    queueIndices.graphicsFamily.value(), 
    queueIndices.presentFamily.value()
  };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
  }

  // set device features
  VkPhysicalDeviceFeatures deviceFeatures{};

  // get device extensions
  auto extensions = getRequiredExtensions();

  // device info
  VkDeviceCreateInfo deviceInfo{};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceInfo.pEnabledFeatures = &deviceFeatures;
  deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  deviceInfo.ppEnabledExtensionNames = extensions.data();

  if (vkCreateDevice(physical, &deviceInfo, nullptr, &logical) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to create logical device");
  }
}

/**
 * @brief retrieve the Vulkan device extensions necessary for the app
 * 
 * @return std::vector<const char*> 
 */
std::vector<const char*> Device::getRequiredExtensions() {
  std::vector<const char*> extensions(deviceExtensions);
  return extensions;
}

/**
 * @brief retrieve the necessary queues with the selected indices
 * 
 */
void Device::getQueues() {
  vkGetDeviceQueue(logical, queueIndices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(logical, queueIndices.presentFamily.value(), 0, &presentQueue);
}

}