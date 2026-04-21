#ifndef H_TYPES
#define H_TYPES

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef struct {
  VkSwapchainKHR handle;
  VkExtent2D extent;
  VkFormat imageFormat;
  uint32_t imageCount;
  VkImage* images;
  // Tells which part of an image should be accessed, and how it should be accessed
  VkImageView* imageViews;
} Swapchain;

typedef struct {
  VkSemaphore imageAvailableSemaphores;
  VkSemaphore renderFinishedSemaphores;
  VkFence inFlightFences;
} Frame;

#endif