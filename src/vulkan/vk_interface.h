#pragma once 

#include "window.h"
#include "device.h"
#include "swapchain.h"
#include "descriptors.h"
#include "pipeline_builder.h"

#include "../util/vk_mem_alloc.h"

#include <memory>
#include <vector>
#include <iostream>

#include <vulkan/vulkan_core.h>

const std::vector<const char*> validationLayers = {
  "VK_LAYER_KHRONOS_validation"
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
class VKInterface {
public:
  // vulkan and sdl handlers
  std::unique_ptr<Window> window;
  VkInstance instance;
  VmaAllocator allocator;
  VkSurfaceKHR surface;
  std::shared_ptr<Device> device;
  std::unique_ptr<Swapchain> swapchain;
  std::unique_ptr<Descriptors> descriptors;
  std::unique_ptr<PipelineBuilder> pipelineBuilder;

  VKInterface(){}
  ~VKInterface();

  void init(uint32_t width = 900, uint32_t height = 600, const unsigned int FRAME_COUNT = 2);
private:
  // interface states
  bool initialized = false;
  void terminate();

  // VK instance related functions
  void createInstance();
  std::vector<const char*> getRequiredExtensions();
  bool checkValidationLayerSupport();
  void createAllocator();

  // other VK handlers
  void createSurface();

  // debug related functions
  VkDebugUtilsMessengerEXT debugMessenger;
  void setupDebugMessenger();

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