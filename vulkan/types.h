#ifndef H_TYPES
#define H_TYPES

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

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
  VkSemaphore* imageAvailableSemaphores;
  VkSemaphore* renderFinishedSemaphores;
  VkFence* inFlightFences;
} SyncObjects;

typedef struct {
  float pos[2];
  float color[3];
} Vertex;

// typedef struct {
//   float model[4][4];
//   float view[4][4];
//   float proj[4][4];
// } UniformBufferObject;

typedef struct {
  mat4 model;
  mat4 view;
  mat4 proj;
} UniformBufferObject;

#endif