#ifndef H_TYPES
#define H_TYPES

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define MAX_FRAMES_IN_FLIGHT 2

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

typedef struct {
  vec3 pos;
  vec3 front;
  vec3 right;
  float yaw;
  float pitch;
  float speed;
  float fov;
} Camera;

typedef struct {
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  // Swapchain owns the framebuffers. Essentially a queue of images. The general purpose of the swap
  // chain is to synchronize the presentation of images with the refresh rate of the screen.
  Swapchain swapchain;
  SyncObjects syncObjects;
  // Buffer for recording Vulkan commands
  VkCommandBuffer* commandBuffers;
  void* uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  // Commandbuffers are sent to the queue which is then submitted to the GPU
  VkQueue queue;
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
  GLFWwindow* window;
  Camera* camera;
  bool* framebufferResized;
  vec3* worldUp;
} Engine;

#endif