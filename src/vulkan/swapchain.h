#pragma once

#include <vector>

#include <SDL_video.h>
#include <vulkan/vulkan_core.h>
#include <SDL_vulkan.h>

#include "../util/types.h"

namespace mb {

/**
 * @brief 
 * 
 */
class Swapchain {
public:
  Swapchain(const VkInstance _instance, VkDevice _device, VkPhysicalDevice _physicalDevice, 
    QueueFamilyIndices _indices, const VkSurfaceKHR _surface, SDL_Window* _window);
  ~Swapchain();

  VkSwapchainKHR get() {return swapchain;}

  void recreate();

  // vulkan handles
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  VkRenderPass renderPass;
  std::vector<VkFramebuffer> framebuffers;

  // chosen swapchain settings
  VkSurfaceFormatKHR swapchainFormat;
  VkPresentModeKHR swapchainPresentMode;
  VkExtent2D swapchainExtent;
private:
  // vulkan handlers initialized in other classes
  const VkInstance instance;
  const VkPhysicalDevice physicalDevice;
  const VkDevice device;
  const VkSurfaceKHR surface;
  QueueFamilyIndices queueIndices;
  SDL_Window* window;

  // handle for swapchain
  VkSwapchainKHR swapchain;

  // avaiable details about the swapchain
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;

  void getSwapchainDetails();
  void chooseSwapchainSettings();
  void createSwapchain();
  void createImages();
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  void cleanup();
};

}