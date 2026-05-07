#ifndef H_INIT
#define H_INIT

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>

#include "init_buffer.h"
#include "init_models.h"
#include "init_pipeline.h"
#include "init_swapchain.h"
#include "init_uniform_buffers.h"
#include "types.h"

void CHECK(int x) {
  if ((x) != VK_SUCCESS) {
    printf("Vulkan error: %d\n", x);
    exit(1);
  }
}

void createInstance(VkInstance* instance) {
  VkApplicationInfo applicationInfo = {.apiVersion = VK_API_VERSION_1_3};

  /**
   * GLFW requires the cross-platform extension, VkSurfaceKHR, and it's native
   * counterpart, which on Wayland system is VK_KHR_wayland_surface. These
   * belong to the WSI (Window System Integration) extensions, that provide an
   * abstraction over the native window systems.
   */
  uint32_t glfwExtensionsCount;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

  VkInstanceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &applicationInfo,
    .enabledExtensionCount = glfwExtensionsCount,
    .ppEnabledExtensionNames = glfwExtensions,
  };
  CHECK(vkCreateInstance(&info, NULL, instance));
}

int findSuitableQueueFamily(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice) {
  uint32_t index;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &index, NULL);
  VkQueueFamilyProperties families[index];
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &index, families);

  for (uint32_t i = 0; i < index; i++) {
    VkBool32 supportsPresentation = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresentation);
    if (supportsPresentation && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      return i;
    }
  }
  return -1;
}

int pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                       VkPhysicalDevice* physicalDevice) {
  uint32_t physicalDevicesCount;
  vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, NULL);
  VkPhysicalDevice physicalDevices[physicalDevicesCount];
  vkEnumeratePhysicalDevices(instance, &physicalDevicesCount, physicalDevices);

  int queueFamilyIndex = -1;
  for (uint32_t i = 0; i < physicalDevicesCount; i++) {
    queueFamilyIndex = findSuitableQueueFamily(surface, physicalDevices[i]);
    if (queueFamilyIndex != -1) {
      *physicalDevice = physicalDevices[i];
      return queueFamilyIndex;
    };
  }
  return -1;
}

void createLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                         VkDevice* device) {
  VkPhysicalDeviceVulkan13Features enabledVk13Features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .synchronization2 = VK_TRUE,
    .dynamicRendering = VK_TRUE,
  };

  VkPhysicalDeviceVulkan11Features enabledVk11Features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    .pNext = &enabledVk13Features,
    .shaderDrawParameters = true,
  };

  // VkPhysicalDeviceFeatures enabledVk10Features = {.sampleRateShading = true};

  float priority = 1.0f;

  const char* enabledExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &enabledVk11Features,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = (VkDeviceQueueCreateInfo[]){{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,  // number of queues to create in the queue family
      .pQueuePriorities = &priority,
    }},
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = enabledExtensionNames,
    // .pEnabledFeatures = &enabledVk10Features,
  };

  CHECK(vkCreateDevice(physicalDevice, &info, NULL, device));
}

void createSyncObjects(VkDevice device, uint32_t swapchainImageCount, SyncObjects* syncObjects) {
  VkSemaphoreCreateInfo semaphoreCI = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceCI = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  syncObjects->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    CHECK(vkCreateSemaphore(device, &semaphoreCI, NULL, &syncObjects->renderFinishedSemaphores[i]));
  }

  syncObjects->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  syncObjects->inFlightFences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CHECK(vkCreateSemaphore(device, &semaphoreCI, NULL, &syncObjects->imageAvailableSemaphores[i]));
    CHECK(vkCreateFence(device, &fenceCI, NULL, &syncObjects->inFlightFences[i]));
  }
}

void initVulkan(GLFWwindow* window, VkSurfaceKHR* surface, VkPhysicalDevice* physicalDevice,
                VkDevice* device, Swapchain* swapchain, SyncObjects* syncObjects,
                VkCommandBuffer** commandBuffersPtr, void** uniformBuffersMapped,
                VkDescriptorSetLayout* descriptorSetLayout, VkDescriptorSet* descriptorSets,
                VkPipelineLayout* pipelineLayout, VkPipeline* graphicsPipeline, VkQueue* queue,
                VkBuffer* vertexBuffer, VkBuffer* indexBuffer) {
  VkInstance instance;
  createInstance(&instance);

  CHECK(glfwCreateWindowSurface(instance, window, NULL, surface));

  uint32_t queueFamilyIndex = pickPhysicalDevice(instance, *surface, physicalDevice);
  createLogicalDevice(*physicalDevice, queueFamilyIndex, device);

  createSwapchain(window, *surface, *physicalDevice, *device, swapchain);

  createSyncObjects(*device, swapchain->imageCount, syncObjects);

  VkCommandPoolCreateInfo commandPoolCI = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndex,
  };
  VkCommandPool commandPool;
  CHECK(vkCreateCommandPool(*device, &commandPoolCI, NULL, &commandPool));
  *commandBuffersPtr = createCommandBuffers(*device, commandPool, MAX_FRAMES_IN_FLIGHT);

  initUniformBuffers(physicalDevice, device, uniformBuffersMapped, descriptorSetLayout,
                     descriptorSets);

  createPipeline(*device, swapchain, *descriptorSetLayout, pipelineLayout, graphicsPipeline);

  vkGetDeviceQueue(*device, queueFamilyIndex, 0, queue);
  createVertexBuffer(*device, *physicalDevice, commandPool, *queue, vertexBuffer);
  createIndexBuffer(*device, *physicalDevice, commandPool, *queue, indexBuffer);
}

Engine* init(GLFWwindow* window) {
  Engine* ng = malloc(sizeof(Engine));
  ng->window = window;

  initVulkan(window, &ng->surface, &ng->physicalDevice, &ng->device, &ng->swapchain,
             &ng->syncObjects, &ng->commandBuffers, ng->uniformBuffersMapped,
             &ng->descriptorSetLayout, ng->descriptorSets, &ng->pipelineLayout,
             &ng->graphicsPipeline, &ng->queue, &ng->vertexBuffer, &ng->indexBuffer);

  return ng;
}

void handleWindowResize(Engine* ng) {
  vkDeviceWaitIdle(ng->device);
  vkDestroySwapchainKHR(ng->device, ng->swapchain.handle, NULL);
  createSwapchain(ng->window, ng->surface, ng->physicalDevice, ng->device, &ng->swapchain);
}

#endif