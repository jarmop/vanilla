#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "util.h"

#define MAX_FRAMES_IN_FLIGHT 2

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

size_t currentFrame = 0;
bool framebufferResized = false;

void CHECK(int x) {
  if ((x) != VK_SUCCESS) {
    printf("Vulkan error: %d\n", x);
    exit(1);
  }
}

static char* readFile(const char* filename, size_t* size) {
  FILE* f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);

  char* data = malloc(*size);
  fread(data, 1, *size, f);
  fclose(f);
  return data;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)scancode;
  (void)mods;
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
  (void)window;
  (void)width;
  (void)height;
  framebufferResized = true;
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
  // for (uint32_t i = 0; i < glfwExtensionsCount; i++) {
  //   fprintf(stderr, "%s\n", glfwExtensions[i]);
  // }

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

void createSwapchain(GLFWwindow* window, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice,
                     VkDevice device, Swapchain* swapchain) {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  VkSurfaceFormatKHR surfaceFormat = chooseImageFormat(physicalDevice, surface);
  VkSwapchainCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = chooseMinImageCount(&surfaceCapabilities),
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = chooseSurfaceExtent(window, &surfaceCapabilities),
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = surfaceCapabilities.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
  };
  CHECK(vkCreateSwapchainKHR(device, &info, NULL, &swapchain->handle));

  swapchain->imageFormat = info.imageFormat;
  swapchain->extent = info.imageExtent;

  vkGetSwapchainImagesKHR(device, swapchain->handle, &swapchain->imageCount, NULL);
  swapchain->images = malloc(sizeof(VkImage) * swapchain->imageCount);
  vkGetSwapchainImagesKHR(device, swapchain->handle, &swapchain->imageCount, swapchain->images);

  swapchain->imageViews = malloc(sizeof(VkImageView) * swapchain->imageCount);
  for (uint32_t i = 0; i < swapchain->imageCount; i++) {
    VkImageViewCreateInfo view = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swapchain->images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = swapchain->imageFormat,
      // describes what the image’s purpose is and which part of the image should be accessed
      .subresourceRange =
        {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
        },
    };
    CHECK(vkCreateImageView(device, &view, NULL, &swapchain->imageViews[i]));
  }
}

VkShaderModule createShaderModule(const char* file, VkDevice device) {
  size_t size;
  char* code = readFile(file, &size);
  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = (uint32_t*)code,
  };
  VkShaderModule module;
  CHECK(vkCreateShaderModule(device, &info, NULL, &module));
  free(code);
  return module;
}

void createPipeline(VkDevice device, Swapchain* swapchain, VkPipeline* graphicsPipeline) {
  VkPipelineRenderingCreateInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = (VkFormat[]){swapchain->imageFormat},
  };

  VkShaderModule triangle = createShaderModule("shaders/out.spv", device);
  VkPipelineShaderStageCreateInfo stages[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = triangle,
      .pName = "vertMain",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = triangle,
      .pName = "fragMain",
    },
  };

  const VkVertexInputBindingDescription vertexBindingDescriptions[] = {{
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  }};
  VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
    {
      .location = 0,
      .binding = 0,
      .format = VK_FORMAT_R32G32_SFLOAT,
      .offset = offsetof(Vertex, pos),
    },
    {
      .location = 1,
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(Vertex, color),
    },
  };
  VkPipelineVertexInputStateCreateInfo vertexInputState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = vertexBindingDescriptions,
    .vertexAttributeDescriptionCount = 2,
    .pVertexAttributeDescriptions = vertexAttributeDescriptions,
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  VkPipelineViewportStateCreateInfo viewportState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = (VkViewport[]){{
      .width = (float)swapchain->extent.width,
      .height = (float)swapchain->extent.height,
      .maxDepth = 1.0f,
    }},
    .scissorCount = 1,
    .pScissors = (VkRect2D[]){{{0, 0}, swapchain->extent}},
  };

  VkPipelineRasterizationStateCreateInfo rasterizationState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
  };

  VkPipelineMultisampleStateCreateInfo multiSampleState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    // .sampleShadingEnable = VK_TRUE,
  };

  VkPipelineColorBlendStateCreateInfo colorBlendState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = (VkPipelineColorBlendAttachmentState[]){{
      // .colorWriteMask = 0xF,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    }},
  };

  VkPipelineLayoutCreateInfo layoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  VkPipelineLayout layout;
  CHECK(vkCreatePipelineLayout(device, &layoutInfo, NULL, &layout));

  VkGraphicsPipelineCreateInfo pipe = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &renderingInfo,
    .stageCount = 2,
    .pStages = stages,  // The stages programmed by our shaders: vertex and fragment
    .pVertexInputState = &vertexInputState,
    .pInputAssemblyState = &inputAssemblyState,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizationState,
    .pMultisampleState = &multiSampleState,
    .pColorBlendState = &colorBlendState,
    .layout = layout,
  };
  CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, NULL, graphicsPipeline));

  vkDestroyShaderModule(device, triangle, NULL);
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  fprintf(stderr, "failed to find suitable memory type\n");
  exit(1);
}

VkBuffer vertexBuffer;
VkBuffer indexBuffer;

// clang-format off
const Vertex vertices[] = {
  {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
  {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
  {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
  {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
};
// clang-format on
int indexCount = 6;
const uint32_t indices[] = {0, 1, 2, 2, 3, 0};

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize,
                  VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memPropFlags,
                  VkBuffer* buffer, VkDeviceMemory* bufferMemory) {
  VkBufferCreateInfo bufferInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = bufferSize,
    .usage = usageFlags,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  if (vkCreateBuffer(device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
    perror("vkCreateBuffer");
  }
  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);
  VkMemoryAllocateInfo allocInfoStaging = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memRequirements.size,
    .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memPropFlags),
  };
  if (vkAllocateMemory(device, &allocInfoStaging, NULL, bufferMemory) != VK_SUCCESS) {
    perror("vkAllocateMemory");
  }
  vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

void copyBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkBuffer srcBuffer,
                VkBuffer dstBuffer, VkDeviceSize bufferSize) {
  VkCommandBufferAllocateInfo comBufAlloc = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  VkCommandBuffer* comCopyBuffers =
    malloc(sizeof(VkCommandBuffer) * comBufAlloc.commandBufferCount);
  CHECK(vkAllocateCommandBuffers(device, &comBufAlloc, comCopyBuffers));
  VkCommandBuffer comCopyBuffer = comCopyBuffers[0];
  VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(comCopyBuffer, &begin);
  VkBufferCopy region = {.srcOffset = 0, .dstOffset = 0, .size = bufferSize};
  vkCmdCopyBuffer(comCopyBuffer, srcBuffer, dstBuffer, 1, &region);
  vkEndCommandBuffer(comCopyBuffer);
  VkSubmitInfo submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = comCopyBuffers,
  };
  vkQueueSubmit(queue, 1, &submit, NULL);
  vkQueueWaitIdle(queue);
}

void createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue,
                        VkCommandPool commandPool) {
  VkDeviceSize bufferSize = sizeof(vertices);

  // STAGING BUFFER
  VkBuffer vertexBufferStaging;
  VkDeviceMemory vertexBufferMemoryStaging;
  createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &vertexBufferStaging, &vertexBufferMemoryStaging);

  void* data;
  vkMapMemory(device, vertexBufferMemoryStaging, 0, bufferSize, 0, &data);
  memcpy(data, vertices, (size_t)bufferSize);
  vkUnmapMemory(device, vertexBufferMemoryStaging);

  // FINAL BUFFER
  VkDeviceMemory vertexBufferMemory;
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               //  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

  // Copy staging to final
  copyBuffer(device, queue, commandPool, vertexBufferStaging, vertexBuffer, bufferSize);
}

void createIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue,
                       VkCommandPool commandPool) {
  VkDeviceSize bufferSize = sizeof(indices);

  // STAGING BUFFER
  VkBuffer indexBufferStaging;
  VkDeviceMemory indexBufferMemoryStaging;
  createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &indexBufferStaging, &indexBufferMemoryStaging);

  void* data;
  vkMapMemory(device, indexBufferMemoryStaging, 0, bufferSize, 0, &data);
  memcpy(data, indices, (size_t)bufferSize);
  vkUnmapMemory(device, indexBufferMemoryStaging);

  // FINAL BUFFER
  VkDeviceMemory indexBufferMemory;
  createBuffer(device, physicalDevice, bufferSize,
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               //  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

  // Copy staging to final
  copyBuffer(device, queue, commandPool, indexBufferStaging, indexBuffer, bufferSize);
}

void createCommandBuffers(VkDevice device, VkCommandBuffer** commandBuffersPtr,
                          VkCommandPool commandPool) {
  VkCommandBufferAllocateInfo alloc = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };
  VkCommandBuffer* commandBuffers = malloc(sizeof(VkCommandBuffer) * alloc.commandBufferCount);
  CHECK(vkAllocateCommandBuffers(device, &alloc, commandBuffers));

  *commandBuffersPtr = commandBuffers;
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
                VkDevice* device, VkQueue* queue, Swapchain* swapchain,
                VkPipeline* graphicsPipeline, VkCommandBuffer** commandBuffers,
                SyncObjects* syncObjects) {
  VkInstance instance;
  createInstance(&instance);

  CHECK(glfwCreateWindowSurface(instance, window, NULL, surface));

  uint32_t queueFamilyIndex = pickPhysicalDevice(instance, *surface, physicalDevice);
  createLogicalDevice(*physicalDevice, queueFamilyIndex, device);
  vkGetDeviceQueue(*device, queueFamilyIndex, 0, queue);

  createSwapchain(window, *surface, *physicalDevice, *device, swapchain);

  createPipeline(*device, swapchain, graphicsPipeline);

  VkCommandPoolCreateInfo pool = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamilyIndex,
  };
  VkCommandPool commandPool;
  CHECK(vkCreateCommandPool(*device, &pool, NULL, &commandPool));

  createVertexBuffer(*device, *physicalDevice, *queue, commandPool);
  createIndexBuffer(*device, *physicalDevice, *queue, commandPool);

  createCommandBuffers(*device, commandBuffers, commandPool);

  createSyncObjects(*device, swapchain->imageCount, syncObjects);
}

void recordCommandBuffers(uint32_t imageIndex, VkCommandBuffer commandBuffer,
                          VkPipeline graphicsPipeline, Swapchain* swapchain) {
  // Bunch of stuff needs to be based on the imageIndex rather than frame. So even if there is only
  // one command buffer per "frame in flight" (2) the commands are recreated on every image (4)
  VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  // VkCommandBuffer commandBuffer = commandBuffers[i];

  vkBeginCommandBuffer(commandBuffer, &begin);

  VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = swapchain->images[imageIndex],
    .subresourceRange =
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      },
  };
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1,
                       &barrier);

  VkRenderingAttachmentInfo colorAttachmentInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = swapchain->imageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
  };
  VkRenderingInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea = {.extent = swapchain->extent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentInfo,
  };
  vkCmdBeginRendering(commandBuffer, &renderingInfo);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  VkDeviceSize vertexOffset = 0;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexOffset);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

  vkCmdEndRendering(commandBuffer);

  barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  vkEndCommandBuffer(commandBuffer);
}

void drawFrame(VkDevice device, VkQueue queue, Swapchain* swapchain,
               VkCommandBuffer* commandBuffers, VkPipeline graphicsPipeline,
               SyncObjects* syncObjects) {
  vkWaitForFences(device, 1, &syncObjects->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &syncObjects->inFlightFences[currentFrame]);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapchain->handle, UINT64_MAX,
                        syncObjects->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                        &imageIndex);

  VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
  vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  recordCommandBuffers(imageIndex, commandBuffer, graphicsPipeline, swapchain);

  VkSubmitInfo submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = (VkSemaphore[]){syncObjects->imageAvailableSemaphores[currentFrame]},
    .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = (VkSemaphore[]){syncObjects->renderFinishedSemaphores[imageIndex]},
  };
  vkQueueSubmit(queue, 1, &submit, syncObjects->inFlightFences[currentFrame]);

  VkPresentInfoKHR present = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = (VkSemaphore[]){syncObjects->renderFinishedSemaphores[imageIndex]},
    .swapchainCount = 1,
    .pSwapchains = &swapchain->handle,
    .pImageIndices = &imageIndex,
  };
  vkQueuePresentKHR(queue, &present);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Tutorial", NULL, NULL);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  // Commandbuffers are sent to the queue which is then submitted to the GPU
  VkQueue queue;
  // Owns the framebuffers. Essentially a queue of images. The general purpose of the swap chain is
  // to synchronize the presentation of images with the refresh rate of the screen.
  Swapchain swapchain;
  // Buffer for recording Vulkan commands
  VkCommandBuffer* commandBuffers;
  VkPipeline graphicsPipeline;
  SyncObjects syncObjects;

  initVulkan(window, &surface, &physicalDevice, &device, &queue, &swapchain, &graphicsPipeline,
             &commandBuffers, &syncObjects);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame(device, queue, &swapchain, commandBuffers, graphicsPipeline, &syncObjects);

    if (framebufferResized) {
      framebufferResized = false;

      // If window was minimized, Wait until it's expanded again
      int width = 0, height = 0;
      glfwGetFramebufferSize(window, &width, &height);
      while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
      }

      vkDeviceWaitIdle(device);

      vkDestroySwapchainKHR(device, swapchain.handle, NULL);
      createSwapchain(window, surface, physicalDevice, device, &swapchain);
    }
  }

  return 0;
}