#ifndef H_INIT_SWAPCHAIN
#define H_INIT_SWAPCHAIN

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "types.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

static uint32_t chooseMinImageCount(const VkSurfaceCapabilitiesKHR* capabilities) {
  uint32_t minImageCount = max(3u, capabilities->minImageCount);
  if (minImageCount > capabilities->maxImageCount && capabilities->maxImageCount > 0) {
    minImageCount = capabilities->maxImageCount;
  }
  return minImageCount;
}

static VkSurfaceFormatKHR chooseImageFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
  VkSurfaceFormatKHR surfaceFormats[formatCount];
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats);
  VkFormat desiredFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
  // VkFormat desiredFormat = VK_FORMAT_R8G8B8A8_SRGB;
  // Khronos Vulkan tutorial uses this format but it makes the color clearly too light
  // VkFormat desiredFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkColorSpaceKHR desiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  for (uint32_t i = 0; i < formatCount; i++) {
    VkSurfaceFormatKHR f = surfaceFormats[i];
    if (f.format == desiredFormat && f.colorSpace == desiredColorSpace) {
      return f;
    }
  }

  return surfaceFormats[0];
}

static VkExtent2D chooseSurfaceExtent(GLFWwindow* window,
                                      const VkSurfaceCapabilitiesKHR* capabilities) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  }

  int fbWidth, fbHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

  VkExtent2D minE = capabilities->minImageExtent;
  VkExtent2D maxE = capabilities->maxImageExtent;

  return (VkExtent2D){
    min(max((uint32_t)fbWidth, minE.width), maxE.width),
    min(max((uint32_t)fbHeight, minE.height), maxE.height),
  };
}

VkSwapchainCreateInfoKHR swapchainCI = {
  .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
  .imageArrayLayers = 1,
  .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
  .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
  .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
  .presentMode = VK_PRESENT_MODE_FIFO_KHR,
  .clipped = VK_TRUE,
};

VkImageViewCreateInfo imaageViewCI = {
  .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
  .viewType = VK_IMAGE_VIEW_TYPE_2D,
  // describes what the image’s purpose is and which part of the image should be accessed
  .subresourceRange =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
};

void createSwapchain(GLFWwindow* window, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice,
                     VkDevice device, Swapchain* swapchain) {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  swapchain->extent = chooseSurfaceExtent(window, &surfaceCapabilities);

  VkSurfaceFormatKHR surfaceFormat = chooseImageFormat(physicalDevice, surface);
  swapchain->imageFormat = surfaceFormat.format;

  swapchainCI.surface = surface;
  swapchainCI.minImageCount = chooseMinImageCount(&surfaceCapabilities);
  swapchainCI.imageFormat = swapchain->imageFormat;
  swapchainCI.imageColorSpace = surfaceFormat.colorSpace;
  swapchainCI.imageExtent = swapchain->extent;
  swapchainCI.preTransform = surfaceCapabilities.currentTransform;
  vkCreateSwapchainKHR(device, &swapchainCI, NULL, &swapchain->handle);

  vkGetSwapchainImagesKHR(device, swapchain->handle, &swapchain->imageCount, NULL);
  swapchain->images = malloc(sizeof(VkImage) * swapchain->imageCount);
  vkGetSwapchainImagesKHR(device, swapchain->handle, &swapchain->imageCount, swapchain->images);

  swapchain->imageViews = malloc(sizeof(VkImageView) * swapchain->imageCount);
  imaageViewCI.format = swapchain->imageFormat;
  for (uint32_t i = 0; i < swapchain->imageCount; i++) {
    imaageViewCI.image = swapchain->images[i];
    vkCreateImageView(device, &imaageViewCI, NULL, &swapchain->imageViews[i]);
  }
}

#endif