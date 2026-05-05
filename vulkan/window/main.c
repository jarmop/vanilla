#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>

#define MAX_FRAMES_IN_FLIGHT 1

GLFWwindow* window;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkSwapchainKHR swapchain;
VkExtent2D swapchainExtent;
VkImage* swapchainImages;
VkImageView* swapchainImageViews;
VkCommandBuffer commandBuffer;

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

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

void createSwapchain() {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
  VkSurfaceFormatKHR formats[formatCount];
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
  VkSurfaceFormatKHR format = formats[0];
  VkFormat swapchainImageFormat = format.format;
  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = surfaceCapabilities.minImageCount + 1,
    .imageFormat = format.format,
    .imageColorSpace = format.colorSpace,
    .imageExtent = chooseSurfaceExtent(window, &surfaceCapabilities),
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = surfaceCapabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
  };
  vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapchain);

  swapchainExtent = swapchainCreateInfo.imageExtent;

  uint32_t swapchainImageCount;
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
  //   VkImage swapchainImages[swapchainImageCount];
  swapchainImages = malloc(sizeof(VkImage) * swapchainImageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);

  //   VkImageView swapchainImageViews[swapchainImageCount];
  swapchainImageViews = malloc(sizeof(VkImageView) * swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageViewCreateInfo imageViewCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = swapchainImageFormat,
      // describes what the image’s purpose is and which part of the image should be accessed
      .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
    };
    vkCreateImageView(device, &imageViewCreateInfo, NULL, &swapchainImageViews[i]);
  }
}

void recordCommands(uint32_t imageIndex) {
  VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(commandBuffer, &begin);

  VkImageMemoryBarrier2 barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    // .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = swapchainImages[imageIndex],
    .subresourceRange =
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      },
  };
  VkDependencyInfo dependencyInfo = {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &barrier,
  };
  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  VkClearValue clearValue = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
  VkRenderingAttachmentInfo colorAttachmentInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = swapchainImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .clearValue = clearValue,
  };
  VkRenderingInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea = {.extent = swapchainExtent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentInfo,
  };
  vkCmdBeginRendering(commandBuffer, &renderingInfo);
  //   vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
  vkCmdEndRendering(commandBuffer);

  barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  vkEndCommandBuffer(commandBuffer);
}

int main() {
  glfwInit();
  // Disable context creation because Vulkan does not have a context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(800, 600, "Vulkan window", NULL, NULL);

  // CREATE INSTANCE
  VkApplicationInfo applicationInfo = {.apiVersion = VK_API_VERSION_1_3};
  uint32_t glfwExtensionsCount;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
  VkInstanceCreateInfo instanceCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &applicationInfo,
    .enabledExtensionCount = glfwExtensionsCount,
    .ppEnabledExtensionNames = glfwExtensions,
  };
  VkInstance instance;
  vkCreateInstance(&instanceCreateInfo, NULL, &instance);

  // CREATE SURFACE
  glfwCreateWindowSurface(instance, window, NULL, &surface);

  // PICK PHYSICAL DEVICE
  uint32_t physicalDeviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
  VkPhysicalDevice physicalDevices[physicalDeviceCount];
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);
  int queueFamilyIndex = 0;
  physicalDevice = physicalDevices[queueFamilyIndex];

  // CREATE LOGICAL DEVICE
  VkPhysicalDeviceVulkan13Features enabledVk13Features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .synchronization2 = VK_TRUE,
    .dynamicRendering = VK_TRUE,
  };
  float priority = 1.0f;
  const char* enabledExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &enabledVk13Features,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = (VkDeviceQueueCreateInfo[]){{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,  // number of queues to create in the queue family
      .pQueuePriorities = &priority,
    }},
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = enabledExtensionNames,
  };
  vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);

  // CREATE SWAPCHAIN
  createSwapchain();

  // CREATE COMMMAND BUFFERS
  VkCommandPoolCreateInfo commandPoolCI = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndex,
  };
  VkCommandPool commandPool;
  vkCreateCommandPool(device, &commandPoolCI, NULL, &commandPool);
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };
  VkCommandBuffer* commandBuffers =
    malloc(sizeof(VkCommandBuffer) * commandBufferAllocateInfo.commandBufferCount);
  vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers);
  commandBuffer = commandBuffers[0];

  // GET DEVICE QUEUE
  VkQueue queue;
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

  // CREATE SYNC OBJECTS
  VkSemaphoreCreateInfo semaphoreCI = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkSemaphore imageAvailableSemaphore;
  vkCreateSemaphore(device, &semaphoreCI, NULL, &imageAvailableSemaphore);
  VkSemaphore renderFinishedSemaphore;
  vkCreateSemaphore(device, &semaphoreCI, NULL, &renderFinishedSemaphore);
  VkFenceCreateInfo fenceCI = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  VkFence fence;
  vkCreateFence(device, &fenceCI, NULL, &fence);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE,
                          &imageIndex);

    recordCommands(imageIndex);

    vkQueueWaitIdle(queue);

    VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = (VkSemaphore[]){imageAvailableSemaphore},
      .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = (VkSemaphore[]){renderFinishedSemaphore},
    };
    vkQueueSubmit(queue, 1, &submitInfo, fence);

    VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = (VkSemaphore[]){renderFinishedSemaphore},
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
    };
    vkQueuePresentKHR(queue, &presentInfo);
  }

  return 0;
}