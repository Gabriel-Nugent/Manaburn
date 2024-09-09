#pragma once 

#include "window.h"
#include "swapchain.h"

#include "../util/vk_mem_alloc.h"
#include "../util/types.h"

#include <memory>
#include <vector>
#include <iostream>

#include <vulkan/vulkan_core.h>

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation",
};

/**
 * @brief Vulkan device extensions required by the app
 * 
 */
const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

namespace mb {

/**
 * @brief Handles all communication between app and Vulkan
 * 
 */
class vk {
public:
  // vulkan and sdl handlers
  inline static std::unique_ptr<Window> window;
  inline static VkInstance instance;
  inline static VmaAllocator allocator;
  inline static VkSurfaceKHR surface;
  inline static std::unique_ptr<Swapchain> swapchain;

  // vulkan device handlers
  inline static VkPhysicalDevice physicalDevice;
  inline static VkDevice device;
  inline static QueueFamilyIndices queueIndices;
  inline static VkQueue graphicsQueue;
  inline static VkQueue presentQueue;

  vk(){initialized = false;}
  ~vk();

  static void init(uint32_t width = 900, uint32_t height = 600, const unsigned int FRAME_COUNT = 2);
private:
  // interface states
  inline static bool initialized;
  static void terminate();

  // VK instance related functions
  static void createInstance();
  static std::vector<const char*> getRequiredExtensions();
  static bool checkValidationLayerSupport();
  static void createAllocator();

  // other VK handlers
  static void createSurface();

  // VK device related functions
  static void createDevice();
  static void pickPhysicalDevice();
  static bool isDeviceSuitable(VkPhysicalDevice device);
  static void getQueueFamilies();
  static void createLogicalDevice();
  static std::vector<const char*> getRequiredDeviceExtensions();
  static void getQueues();

  // debug related functions
  inline static VkDebugUtilsMessengerEXT debugMessenger;
  static void setupDebugMessenger();

  static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
      if (func != nullptr) {
          return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
      } else {
          return VK_ERROR_EXTENSION_NOT_PRESENT;
      }
    }

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << "\n\n";

    return VK_FALSE;
  }

  static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
  }
};

}