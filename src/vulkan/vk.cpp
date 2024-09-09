#include "vk.h"

#define VMA_IMPLEMENTATION
#include "../util/vk_mem_alloc.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <set>

#include <vulkan/vulkan_core.h>
#include <SDL_vulkan.h>
#include <SDL_stdinc.h>

namespace mb {

  vk::~vk() {
    if (initialized) terminate();
  }

  /**
   * @brief initializes all vulkan and SDL handlers
   * 
   * @param width : the width of the window
   * @param height : the height of the window
   */
  void vk::init(uint32_t width, uint32_t height, const unsigned int FRAME_COUNT) {
    // initalize window
    VkExtent2D extent{width, height};
    window = std::make_unique<Window>();
    window->init(extent);

    // initalize vulkan handlers
    createInstance();
    if (enableValidationLayers) setupDebugMessenger();
    createSurface();
    createDevice();
    createAllocator();
    swapchain = std::make_unique<Swapchain>(instance,device,physicalDevice,queueIndices,surface,window->instance);

    initialized = true;
  }

  /**
   * @brief free all memory and teardown all vulkan handlers
   * 
   */
  void vk::terminate() {
    if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // destroy vulkan handlers in the correct order
    swapchain.reset();
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    window.reset();
  }

  /**
   * @brief creates a new Vulkan API instance
   * 
   */
  void vk::createInstance() {

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
  std::vector<const char*> vk::getRequiredExtensions() {
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window->instance,&sdlExtensionCount,NULL);
    std::vector<const char*> sdlExtensions(sdlExtensionCount); 
    SDL_Vulkan_GetInstanceExtensions(window->instance,&sdlExtensionCount,sdlExtensions.data());

    std::vector<const char*> extensions(sdlExtensions);

    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
  }
  
  /**
   * @brief checks if the requested validation layers are supported
   * 
   * @return true if validation layers are supported
   * @return false if validation layers are not supported
   */
  bool vk::checkValidationLayerSupport() {
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
  void vk::createAllocator() {
    VmaAllocatorCreateInfo allocatorCreateInfo {};
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
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
  void vk::createSurface() {
    if (SDL_Vulkan_CreateSurface(window->instance, instance, &surface) != SDL_TRUE) {
      throw std::runtime_error("[ERROR]: failed to create window surface");
    }
  }

  void vk::createDevice() {
    pickPhysicalDevice();
    getQueueFamilies();
    createLogicalDevice();
    getQueues();
  }

  /**
  * @brief search for GPUs and select the best one for the application
  * 
  */
  void vk::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
      throw std::runtime_error("[ERROR]: failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
      if (isDeviceSuitable(device)) {
        physicalDevice = device;
        break;
      }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
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
  bool vk::isDeviceSuitable(VkPhysicalDevice device) {
    return true;
  }

  /**
  * @brief finds the indices of the queue families for the selected GPU
  * 
  */
  void vk::getQueueFamilies() {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
      // check for graphics support
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
      }
      // check for present support
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
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
  void vk::createLogicalDevice() {
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
    auto extensions = getRequiredDeviceExtensions();

    // device info
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create logical device");
    }
  }

  /**
  * @brief retrieve the Vulkan device extensions necessary for the app
  * 
  * @return std::vector<const char*> 
  */
  std::vector<const char*> vk::getRequiredDeviceExtensions() {
    std::vector<const char*> extensions(deviceExtensions);
    return extensions;
  }

  /**
  * @brief retrieve the necessary queues with the selected indices
  * 
  */
  void vk::getQueues() {
    vkGetDeviceQueue(device, queueIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueIndices.presentFamily.value(), 0, &presentQueue);
  }

  /**
   * @brief set up message callback for vulkan validation layers
   * 
   */
  void vk::setupDebugMessenger() {
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