#define VOLK_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk.h>
#include <vulkan/vulkan.h>

#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

constexpr uint32_t maxFramesInFlight = 1;
uint32_t imageIndex = 0;
uint32_t frameIndex = 0;
VkInstance instance{VK_NULL_HANDLE};
VkDevice device{VK_NULL_HANDLE};
VkQueue queue{VK_NULL_HANDLE};
VkSurfaceKHR surface{VK_NULL_HANDLE};
bool updateSwapchain{false};
VkSwapchainKHR swapchain{VK_NULL_HANDLE};
VkCommandPool commandPool{VK_NULL_HANDLE};
VkPipeline pipeline{VK_NULL_HANDLE};
VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
VmaAllocator allocator{VK_NULL_HANDLE};
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
std::array<VkFence, maxFramesInFlight> fences;
std::array<VkSemaphore, maxFramesInFlight> presentSemaphores;
std::vector<VkSemaphore> renderSemaphores;
VmaAllocation vBufferAllocation{VK_NULL_HANDLE};
VkBuffer vBuffer{VK_NULL_HANDLE};
struct ShaderData {
  glm::mat4 model;
} shaderData{};
struct ShaderDataBuffer {
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};
  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceAddress deviceAddress{};
};
std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
glm::ivec2 windowSize{};
struct Vertex {
  glm::vec3 pos;
};

static inline void chk(VkResult result) {
  if (result != VK_SUCCESS) {
    std::cerr << "Vulkan call returned an error (" << result << ")\n";
    exit(result);
  }
}
static inline void chkSwapchain(VkResult result) {
  if (result < VK_SUCCESS) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      updateSwapchain = true;
      return;
    }
    std::cerr << "Vulkan call returned an error (" << result << ")\n";
    exit(result);
  }
}
static inline void chk(bool result) {
  if (!result) {
    std::cerr << "Call returned an error\n";
    exit(result);
  }
}

static uint8_t* read_file(const char* path, size_t* out_size) {
  FILE* f = fopen(path, "rb");
  if (!f) {
    perror(path);
    exit(1);
  }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz <= 0) {
    fprintf(stderr, "Empty file: %s\n", path);
    exit(1);
  }
  uint8_t* buf = (uint8_t*)malloc((size_t)sz);
  if (!buf) {
    fprintf(stderr, "OOM\n");
    exit(1);
  }
  if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
    fprintf(stderr, "Read failed\n");
    exit(1);
  }
  fclose(f);
  *out_size = (size_t)sz;
  return buf;
}

int main(int argc, char* argv[]) {
  chk(SDL_Init(SDL_INIT_VIDEO));
  chk(SDL_Vulkan_LoadLibrary(NULL));
  volkInitialize();

// Initialization:
// Before using Vulkan, an application must initialize it by loading the Vulkan commands, and
// creating a VkInstance object.
// https://vulkan.lunarg.com/doc/view/1.3.275.0/linux/1.3/vkspec.html#initialization

// Create the Vulkan instance
#pragma region
  VkApplicationInfo appInfo{.apiVersion = VK_API_VERSION_1_3};
  // Get the list of extensions required for rendering vulkan to an SDL window
  uint32_t instanceExtensionsCount{0};
  char const* const* instanceExtensions{SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};
  VkInstanceCreateInfo instanceCI{
    .pApplicationInfo = &appInfo,
    .enabledExtensionCount = instanceExtensionsCount,
    .ppEnabledExtensionNames = instanceExtensions,
  };
  chk(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);
#pragma endregion

// Select a physical device to use for rendering
#pragma region
  uint32_t deviceCount{0};
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> devices(deviceCount);
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
  uint32_t deviceIndex{0};
  if (argc > 1) {
    deviceIndex = std::stoi(argv[1]);
    assert(deviceIndex < deviceCount);
  }
  VkPhysicalDeviceProperties2 deviceProperties{};
  vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";
#pragma endregion

/**
 * Find a queue family for graphics from the physical device
 *
 * Vulkan exposes one or more devices, each of which exposes one or more queues which are
 * partitioned into families which suppport one or more types of functionality.
 * Types of functionality: graphics, compute, protected memory management, sparse memory management,
 * and transfer.
 *
 * https://vulkan.lunarg.com/doc/view/1.3.275.0/linux/1.3/vkspec.html#fundamentals-execmodel
 */
#pragma region
  uint32_t queueFamilyCount{0};
  vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount,
                                           queueFamilies.data());
  uint32_t queueFamily{0};
  for (size_t i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamily = i;
      break;
    }
  }
  chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex], queueFamily));
#pragma endregion

// Get the device handle and a queue for it
#pragma region
  const float qfpriorities{1.0f};
  VkDeviceQueueCreateInfo queueCI{
    .queueFamilyIndex = queueFamily,
    .queueCount = 1,
    .pQueuePriorities = &qfpriorities,
  };
  VkPhysicalDeviceVulkan13Features enabledVk13Features{
    .synchronization2 = true,
  };
  VkPhysicalDeviceVulkan12Features enabledVk12Features{
    .pNext = &enabledVk13Features,
    .descriptorIndexing = true,
    .shaderSampledImageArrayNonUniformIndexing = true,
    .descriptorBindingVariableDescriptorCount = true,
    .runtimeDescriptorArray = true,
    .bufferDeviceAddress = true,
  };
  VkPhysicalDeviceFeatures enabledVk10Features{.samplerAnisotropy = VK_TRUE};
  const std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceCI{
    .pNext = &enabledVk12Features,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueCI,
    .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures = &enabledVk10Features,
  };
  chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));
  vkGetDeviceQueue(device, queueFamily, 0, &queue);
#pragma endregion

// Set up the Vulkan memory allocator
#pragma region
  VmaVulkanFunctions vkFunctions{
    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
    .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    .vkCreateImage = vkCreateImage,
  };
  VmaAllocatorCreateInfo allocatorCI{
    .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice = devices[deviceIndex],
    .device = device,
    .pVulkanFunctions = &vkFunctions,
    .instance = instance,
  };
  chk(vmaCreateAllocator(&allocatorCI, &allocator));
#pragma endregion

// Window and surface
#pragma region
  SDL_Window* window =
    SDL_CreateWindow("How to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(window);
  chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));
  chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));
  VkExtent2D swapchainExtent{surfaceCaps.currentExtent};
  // Special case for Wayland who expects to get the extent (width and height) from the window
  if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
    swapchainExtent = {
      .width = static_cast<uint32_t>(windowSize.x),
      .height = static_cast<uint32_t>(windowSize.y),
    };
  }
#pragma endregion

// Swap chain
#pragma region
  const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB};
  VkSwapchainCreateInfoKHR swapchainCI{
    .surface = surface,
    .minImageCount = surfaceCaps.minImageCount,
    .imageFormat = imageFormat,
    .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
    .imageExtent{.width = swapchainExtent.width, .height = swapchainExtent.height},
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR};
  chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
  uint32_t imageCount{0};
  chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
  swapchainImages.resize(imageCount);
  chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
  swapchainImageViews.resize(imageCount);
  for (int i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo viewCI{
      .image = swapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = imageFormat,
      .subresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      },
    };
    chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
  }
#pragma endregion

// Load vertex and index data
#pragma region
  std::vector<Vertex> vertices{
    Vertex{.pos{0.0, 0.0, 0.0}},
    Vertex{.pos{0.5, 0.0, 0.0}},
    Vertex{.pos{0.0, 0.5, 0.0}},
  };
  const VkDeviceSize indexCount = vertices.size();
  std::vector<uint16_t> indices{0, 1, 2};

  VkDeviceSize vBufSize{sizeof(Vertex) * vertices.size()};
  VkDeviceSize iBufSize{sizeof(uint16_t) * indices.size()};
  VkBufferCreateInfo bufferCI{
    .size = vBufSize + iBufSize,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT};
  VmaAllocationCreateInfo vBufferAllocCI{
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
             VMA_ALLOCATION_CREATE_MAPPED_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO};
  VmaAllocationInfo vBufferAllocInfo{};
  chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation,
                      &vBufferAllocInfo));
  memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
  memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);
#pragma endregion

  // Shader data buffers
  for (auto i = 0; i < maxFramesInFlight; i++) {
    VkBufferCreateInfo uBufferCI{
      .size = sizeof(ShaderData),
      .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    VmaAllocationCreateInfo uBufferAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
    };
    chk(vmaCreateBuffer(allocator, &uBufferCI, &uBufferAllocCI, &shaderDataBuffers[i].buffer,
                        &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo));
    VkBufferDeviceAddressInfo uBufferBdaInfo{.buffer = shaderDataBuffers[i].buffer};
    shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
  }

// Sync objects
#pragma region
  VkSemaphoreCreateInfo semaphoreCI{};
  VkFenceCreateInfo fenceCI{.flags = VK_FENCE_CREATE_SIGNALED_BIT};
  for (auto i = 0; i < maxFramesInFlight; i++) {
    chk(vkCreateFence(device, &fenceCI, nullptr, &fences[i]));
    chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentSemaphores[i]));
  }
  renderSemaphores.resize(swapchainImages.size());
  for (auto& semaphore : renderSemaphores) {
    chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
  }
#pragma endregion

// Command pool
#pragma region
  VkCommandPoolCreateInfo commandPoolCI{
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queueFamily,
  };
  chk(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));
  VkCommandBufferAllocateInfo cbAllocCI{
    .commandPool = commandPool,
    .commandBufferCount = maxFramesInFlight,
  };
  chk(vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data()));
#pragma endregion

// Load the shader, compile it and pass it to the GPU
#pragma region
  const char* path = "assets/shader.spv";
  size_t sz = 0;
  uint8_t* bytes = read_file(path, &sz);
  VkShaderModuleCreateInfo shaderModuleCI{
    .codeSize = sz,
    .pCode = (const uint32_t*)bytes,
  };
  VkShaderModule shaderModule{};
  chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));
#pragma endregion

// PIPELINE
#pragma region
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
    {.stage = VK_SHADER_STAGE_VERTEX_BIT, .module = shaderModule, .pName = "main"},
    {.stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = shaderModule, .pName = "main"},
  };
  VkVertexInputBindingDescription vertexBinding{
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  std::vector<VkVertexInputAttributeDescription> vertexAttributes{
    // Position
    {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
  };
  VkPipelineVertexInputStateCreateInfo vertexInputState{
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBinding,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
    .pVertexAttributeDescriptions = vertexAttributes.data(),
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };
  std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{
    .dynamicStateCount = 2,
    .pDynamicStates = dynamicStates.data(),
  };
  VkPipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
  VkPipelineRasterizationStateCreateInfo rasterizationState{.lineWidth = 1.0f};
  VkPipelineMultisampleStateCreateInfo multisampleState{
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkPipelineColorBlendAttachmentState blendAttachment{.colorWriteMask = 0xF};
  VkPipelineColorBlendStateCreateInfo colorBlendState{
    .attachmentCount = 1,
    .pAttachments = &blendAttachment,
  };
  VkPipelineRenderingCreateInfo renderingCI{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &imageFormat,
  };
  VkGraphicsPipelineCreateInfo pipelineCI{
    .pNext = &renderingCI,
    .stageCount = 2,
    .pStages = shaderStages.data(),
    .pVertexInputState = &vertexInputState,
    .pInputAssemblyState = &inputAssemblyState,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizationState,
    .pMultisampleState = &multisampleState,
    .pColorBlendState = &colorBlendState,
    .pDynamicState = &dynamicState,
    .layout = pipelineLayout,
  };
  chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));
#pragma endregion

  // Render loop
  bool quit = false;
  while (!quit) {
    // Sync
    chk(vkWaitForFences(device, 1, &fences[frameIndex], true, UINT64_MAX));
    chk(vkResetFences(device, 1, &fences[frameIndex]));
    chkSwapchain(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphores[frameIndex],
                                       VK_NULL_HANDLE, &imageIndex));
    // Update shader data
    shaderData.model = glm::mat4(1.0f);
    memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData,
           sizeof(ShaderData));
    // Build command buffer
    auto cb = commandBuffers[frameIndex];
    chk(vkResetCommandBuffer(cb, 0));
    VkCommandBufferBeginInfo cbBI{
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));
    VkRenderingAttachmentInfo colorAttachmentInfo{
      .imageView = swapchainImageViews[imageIndex],
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue{.color{0.0f, 0.0f, 0.0f, 1.0f}},
    };
    VkRenderingInfo renderingInfo{
      .renderArea{.extent{.width = static_cast<uint32_t>(windowSize.x),
                          .height = static_cast<uint32_t>(windowSize.y)}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
    };
    vkCmdBeginRendering(cb, &renderingInfo);
    VkViewport vp{
      .width = static_cast<float>(windowSize.x),
      .height = static_cast<float>(windowSize.y),
    };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{.extent{.width = static_cast<uint32_t>(windowSize.x),
                             .height = static_cast<uint32_t>(windowSize.y)}};
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetScissor(cb, 0, 1, &scissor);
    VkDeviceSize vOffset{0};
    vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, &vOffset);
    vkCmdBindIndexBuffer(cb, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress),
                       &shaderDataBuffers[frameIndex].deviceAddress);
    vkCmdDrawIndexed(cb, indexCount, 3, 0, 0, 0);
    vkCmdEndRendering(cb);
    VkImageMemoryBarrier2 barrierPresent{
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image = swapchainImages[imageIndex],
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
    VkDependencyInfo barrierPresentDependencyInfo{
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrierPresent,
    };
    vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
    chk(vkEndCommandBuffer(cb));
    // Submit to graphics queue
    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &presentSemaphores[frameIndex],
      .pWaitDstStageMask = &waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cb,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &renderSemaphores[imageIndex],
    };
    chk(vkQueueSubmit(queue, 1, &submitInfo, fences[frameIndex]));
    VkPresentInfoKHR presentInfo{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &renderSemaphores[imageIndex],
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
    };
    chkSwapchain(vkQueuePresentKHR(queue, &presentInfo));
    // Event polling
    for (SDL_Event event; SDL_PollEvent(&event);) {
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
        quit = true;
        break;
      }
      // Window resize
      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        updateSwapchain = true;
      }
    }
    if (updateSwapchain) {
      chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
      updateSwapchain = false;
      chk(vkDeviceWaitIdle(device));
      chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));
      swapchainCI.oldSwapchain = swapchain;
      swapchainCI.imageExtent = {.width = static_cast<uint32_t>(windowSize.x),
                                 .height = static_cast<uint32_t>(windowSize.y)};
      chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
      for (auto i = 0; i < imageCount; i++) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
      }
      chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
      swapchainImages.resize(imageCount);
      chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
      swapchainImageViews.resize(imageCount);
      for (auto i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewCI{
          .image = swapchainImages[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = imageFormat,
          .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                               .levelCount = 1,
                               .layerCount = 1},
        };
        chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
      }
      vkDestroySwapchainKHR(device, swapchainCI.oldSwapchain, nullptr);
      VmaAllocationCreateInfo allocCI{.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                      .usage = VMA_MEMORY_USAGE_AUTO};
      VkImageViewCreateInfo viewCI{
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = {.levelCount = 1, .layerCount = 1},
      };
    }
  }

// Tear down
#pragma region
  chk(vkDeviceWaitIdle(device));
  for (auto i = 0; i < maxFramesInFlight; i++) {
    vkDestroyFence(device, fences[i], nullptr);
    vkDestroySemaphore(device, presentSemaphores[i], nullptr);
    vmaDestroyBuffer(allocator, shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
  }
  for (auto i = 0; i < renderSemaphores.size(); i++) {
    vkDestroySemaphore(device, renderSemaphores[i], nullptr);
  }
  for (auto i = 0; i < swapchainImageViews.size(); i++) {
    vkDestroyImageView(device, swapchainImageViews[i], nullptr);
  }
  vmaDestroyBuffer(allocator, vBuffer, vBufferAllocation);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyShaderModule(device, shaderModule, nullptr);
  vmaDestroyAllocator(allocator);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
#pragma endregion
}