#include "swapchain.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <limits>
#include <algorithm>

#include <vulkan/vulkan_core.h>
#include <SDL_vulkan.h>

namespace mb {

Swapchain::Swapchain(const VkInstance _instance, VkDevice _device, VkPhysicalDevice _physicalDevice, 
    QueueFamilyIndices _indices, const VkSurfaceKHR _surface, SDL_Window* _window) :
instance(_instance), device(_device), physicalDevice(_physicalDevice), queueIndices(_indices), surface(_surface), window(_window) {
  getSwapchainDetails();
  chooseSwapchainSettings();
  createSwapchain();
  createImages();
  createImageViews();
  createRenderPass();
  createFramebuffers();
}

Swapchain::~Swapchain() {
  cleanup();
  vkDestroyRenderPass(device, renderPass, nullptr);
}

/**
 * @brief reconstruct the swapchain and all handlers that depend on it
 * 
 */
void Swapchain::recreate() {
  vkDeviceWaitIdle(device);

  cleanup();

  getSwapchainDetails();
  chooseSwapchainSettings();
  createSwapchain();
  createImages();
  createImageViews();
  createFramebuffers();
}

/**
 * @brief queries the GPU for details about the swapchain
 * 
 */
void Swapchain::getSwapchainDetails() {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
  if (formatCount == 0) {
    throw std::runtime_error("[ERROR]: No swapchain formats were found");
  }
  else {
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
  if (presentModeCount == 0) {
    throw std::runtime_error("[ERROR]: No swapchain present modes were found");
  }
  else {
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
  }
}

/**
 * @brief pick predefined swapchain settings
 * 
 */
void Swapchain::chooseSwapchainSettings() {
  // pick a surface format
  bool formatChosen = false;
  for (const auto& availableFormat : formats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapchainFormat = availableFormat;
      formatChosen = true;
    }
  }
  // default if preferred surface format is not available
  if (formatChosen == false) {
    swapchainFormat = formats[0];
  }

  // pick a present mode
  bool presentModeChosen = false;
  for (const auto& availablePresentMode : presentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      swapchainPresentMode = availablePresentMode;
      presentModeChosen = true;
    }
  }
  // default if preferred present mode is not available
  if (presentModeChosen == false) {
    swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  }

  // pick a swap extent
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    swapchainExtent = capabilities.currentExtent;
  }
  else {
    // query SDL for an appropriate swap extent
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);

    VkExtent2D extent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    swapchainExtent = extent;
  }
}

/**
 * @brief create a vulkan swapchain using the previously defined settings
 * 
 */
void Swapchain::createSwapchain() {
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchainInfo{};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = surface;
  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageFormat = swapchainFormat.format;
  swapchainInfo.imageColorSpace = swapchainFormat.colorSpace;
  swapchainInfo.imageExtent = swapchainExtent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {
    queueIndices.graphicsFamily.value(),
    queueIndices.presentFamily.value()
  };

  if (queueIndices.graphicsFamily != queueIndices.presentFamily) {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainInfo.queueFamilyIndexCount = 2;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else { // otherwise the queues require explicit ownership transferring
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;
  }

  swapchainInfo.preTransform = capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = swapchainPresentMode;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: failed to create swapchain");
  }
}

/**
 * @brief retrieves the image handles from the swapchain
 * 
 */
void Swapchain::createImages() {
  uint32_t imageCount;
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  images.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());
}

/**
 * @brief creates image views to access swapchain images
 * 
 */
void Swapchain::createImageViews() {
  imageViews.resize(images.size());
  for (size_t i = 0; i < images.size(); i++) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapchainFormat.format;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create image views");
    }
  }
}

/**
 * @brief creates a render pass object with attachments
 * 
 */
void Swapchain::createRenderPass() {
  VkAttachmentDescription colorAttachment {};
  colorAttachment.format = swapchainFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("[ERROR]: Failed to create render pass");
  }
}

/**
 * @brief create a frame buffer for every image view
 * 
 */
void Swapchain::createFramebuffers() {
  framebuffers.resize(imageViews.size());
  for (size_t i = 0; i < imageViews.size(); i++) {
    VkImageView attachments[] = {
      imageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("[ERROR]: failed to create framebuffer");
    }
  }
}

/**
 * @brief destroy swapchain and all handlers that depend on it
 * 
 */
void Swapchain::cleanup() {
  for (auto& framebuffer : framebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }
  for (auto& view : imageViews) {
    vkDestroyImageView(device, view, nullptr);
  }
  vkDestroySwapchainKHR(device, swapchain, nullptr);
}

}