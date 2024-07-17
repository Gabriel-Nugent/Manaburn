#include "vk_interface.h"

#define VMA_IMPLEMENTATION
#include "../util/vk_mem_alloc.h"

#include <cstdint>
#include <memory>
#include <stdexcept>

#include <vulkan/vulkan_core.h>
#include <SDL_vulkan.h>
#include <SDL_stdinc.h>

namespace mb {

  VKInterface::~VKInterface() {
    if (initialized) terminate();
  }

  /**
   * @brief initializes all vulkan and SDL handlers
   * 
   * @param width : the width of the window
   * @param height : the height of the window
   */
  void VKInterface::init(uint32_t width, uint32_t height, const unsigned int FRAME_COUNT) {
    // initalize window
    VkExtent2D extent{width, height};
    window = std::make_unique<Window>();
    window->init(extent);

    // initalize vulkan handlers
    createInstance();
    if (enableValidationLayers) setupDebugMessenger();
    createSurface();
    device = std::make_shared<Device>(instance,surface);
    createAllocator();
    swapchain = std::make_unique<Swapchain>(instance,device,surface,window->instance);
    descriptors = std::make_unique<Descriptors>(device->logical);

    initialized = true;
  }

  /**
   * @brief free all memory and teardown all vulkan handlers
   * 
   */
  void VKInterface::terminate() {
    if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // destroy vulkan handlers in the correct order
    pipelineBuilder.reset();
    descriptors.reset();
    swapchain.reset();
    vmaDestroyAllocator(allocator);
    device.reset();
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    window.reset();
  }

  /**
   * @brief creates a new Vulkan API instance
   * 
   */
  void VKInterface::createInstance() {

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Manaburn";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.pEngineName = "No Engine"; // app is custom engine
    appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    // add instance extensions
    auto extensions = getRequiredExtensions();
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    // add validation layers
    if (enableValidationLayers && !checkValidationLayerSupport()) {
      throw std::runtime_error("[ERROR]: validation layers requested, but not available");
    }
    else if (enableValidationLayers) {
      instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      instanceInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
      instanceInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create Vulkan instance");
    }
  }

  /**
   * @brief queries for the Vulkan extensions necessary to run the application
   * 
   * @return std::vector<const char*> the extensions that the application uses
   */
  std::vector<const char*> VKInterface::getRequiredExtensions() {
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window->instance,&sdlExtensionCount,NULL);
    std::vector<const char*> sdlExtensions(sdlExtensionCount); 
    SDL_Vulkan_GetInstanceExtensions(window->instance,&sdlExtensionCount,sdlExtensions.data());

    std::vector<const char*> extensions(sdlExtensions);

    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }
  
  /**
   * @brief checks if the requested validation layers are supported
   * 
   * @return true if validation layers are supported
   * @return false if validation layers are not supported
   */
  bool VKInterface::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
      bool layerFound = false;

      for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound) {
          return false;
      }
    }

    return true;
  }

  /**
   * @brief Create handler for VMA allocator
   * 
   */
  void VKInterface::createAllocator() {
    VmaAllocatorCreateInfo allocatorCreateInfo {};
    allocatorCreateInfo.physicalDevice = device->physical;
    allocatorCreateInfo.device = device->logical;
    allocatorCreateInfo.pHeapSizeLimit = nullptr;
    allocatorCreateInfo.pVulkanFunctions = nullptr;
    allocatorCreateInfo.instance = instance;
    
    if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create vma allocator");
    }
  }

  /**
   * @brief create the window surface, an abstraction for our window
   * 
   */
  void VKInterface::createSurface() {
    if (SDL_Vulkan_CreateSurface(window->instance, instance, &surface) != SDL_TRUE) {
      throw std::runtime_error("[ERROR]: failed to create window surface");
    }
  }

  /**
   * @brief set up message callback for vulkan validation layers
   * 
   */
  void VKInterface::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo{};
    messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerInfo.pfnUserCallback = debugCallback;
    messengerInfo.pUserData = nullptr; 

    if (CreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to set up debug messenger");
    }
  }
}