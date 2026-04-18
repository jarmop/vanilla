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
#include <ktx.h>
#include <ktxvulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "slang-com-ptr.h"
#include "slang.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

constexpr uint32_t maxFramesInFlight{2};
uint32_t imageIndex{0};
uint32_t frameIndex{0};
VkInstance instance{VK_NULL_HANDLE};
VkDevice device{VK_NULL_HANDLE};
VkQueue queue{VK_NULL_HANDLE};
VkSurfaceKHR surface{VK_NULL_HANDLE};
bool updateSwapchain{false};
VkSwapchainKHR swapchain{VK_NULL_HANDLE};
VkCommandPool commandPool{VK_NULL_HANDLE};
VkPipeline pipeline{VK_NULL_HANDLE};
VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
VkImage depthImage;
VmaAllocator allocator{VK_NULL_HANDLE};
VmaAllocation depthImageAllocation;
VkImageView depthImageView;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
std::array<VkFence, maxFramesInFlight> fences;
std::array<VkSemaphore, maxFramesInFlight> presentSemaphores;
std::vector<VkSemaphore> renderSemaphores;
VmaAllocation vBufferAllocation{VK_NULL_HANDLE};
VkBuffer vBuffer{VK_NULL_HANDLE};
struct ShaderData {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[3];
  glm::vec4 lightPos{0.0f, -10.0f, 10.0f, 0.0f};
  uint32_t selected{1};
} shaderData{};
struct ShaderDataBuffer {
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};
  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceAddress deviceAddress{};
};
std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
struct Texture {
  VmaAllocation allocation{VK_NULL_HANDLE};
  VkImage image{VK_NULL_HANDLE};
  VkImageView view{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
};
std::array<Texture, 3> textures{};
VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
VkDescriptorSetLayout descriptorSetLayoutTex{VK_NULL_HANDLE};
VkDescriptorSet descriptorSetTex{VK_NULL_HANDLE};
Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
glm::vec3 camPos{0.0f, 0.0f, -6.0f};
glm::ivec2 windowSize{};
struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
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
      .dynamicRendering = true,
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

// Depth attachment for depth testing
#pragma region
  std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
  for (VkFormat& format : depthFormatList) {
    VkFormatProperties2 formatProperties;
    vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format, &formatProperties);
    if (formatProperties.formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      depthFormat = format;
      break;
    }
  }
  assert(depthFormat != VK_FORMAT_UNDEFINED);
  VkImageCreateInfo depthImageCI{
      .imageType = VK_IMAGE_TYPE_2D,
      .format = depthFormat,
      .extent{.width = (uint32_t)windowSize.x, .height = (uint32_t)windowSize.y, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo allocCI{
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
  };
  chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation,
                     nullptr));
  VkImageViewCreateInfo depthViewCI{
      .image = depthImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = depthFormat,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1},
  };
  chk(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView));
#pragma endregion

// Mesh data
#pragma region
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/suzanne.obj"));
  const VkDeviceSize indexCount{shapes[0].mesh.indices.size()};
  std::vector<Vertex> vertices{};
  std::vector<uint16_t> indices{};
#pragma endregion

// Load vertex and index data
#pragma region
  for (auto& index : shapes[0].mesh.indices) {
    Vertex v{
        .pos =
            {
                attrib.vertices[index.vertex_index * 3],
                -attrib.vertices[index.vertex_index * 3 + 1],
                attrib.vertices[index.vertex_index * 3 + 2],
            },
        .normal =
            {
                attrib.normals[index.normal_index * 3],
                -attrib.normals[index.normal_index * 3 + 1],
                attrib.normals[index.normal_index * 3 + 2],
            },
        .uv =
            {
                attrib.texcoords[index.texcoord_index * 2],
                1.0 - attrib.texcoords[index.texcoord_index * 2 + 1],
            },
    };
    vertices.push_back(v);
    indices.push_back(indices.size());
  }
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

// Load texture data to textureDescriptors
#pragma region
  std::vector<VkDescriptorImageInfo> textureDescriptors{};
  for (auto i = 0; i < textures.size(); i++) {
    // Load the image. Why load the same image on every loop?
    ktxTexture* ktxTexture{nullptr};
    std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
    ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                   &ktxTexture);

    // Create the image object and view
    VkImageCreateInfo texImgCI{
        .imageType = VK_IMAGE_TYPE_2D,
        .format = ktxTexture_GetVkFormat(ktxTexture),
        .extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1},
        .mipLevels = ktxTexture->numLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    VmaAllocationCreateInfo texImageAllocCI{.usage = VMA_MEMORY_USAGE_AUTO};
    chk(vmaCreateImage(allocator, &texImgCI, &texImageAllocCI, &textures[i].image,
                       &textures[i].allocation, nullptr));
    VkImageViewCreateInfo texViewCI{
        .image = textures[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = texImgCI.format,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = ktxTexture->numLevels,
                .layerCount = 1,
            },
    };
    chk(vkCreateImageView(device, &texViewCI, nullptr, &textures[i].view));

    // Create a temporary buffer for the image data so the data can be later transferred to the GPU
    VkBuffer imgSrcBuffer{};
    VmaAllocation imgSrcAllocation{};
    VkBufferCreateInfo imgSrcBufferCI{
        .size = (uint32_t)ktxTexture->dataSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    VmaAllocationCreateInfo imgSrcAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};
    VmaAllocationInfo imgSrcAllocInfo{};
    chk(vmaCreateBuffer(allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer,
                        &imgSrcAllocation, &imgSrcAllocInfo));
    memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);

    // Create a buffer for the commands that will transfer the image data to the GPU
    VkFenceCreateInfo fenceOneTimeCI{};
    VkFence fenceOneTime{};
    chk(vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime));
    VkCommandBuffer cbOneTime{};
    VkCommandBufferAllocateInfo cbOneTimeAI{.commandPool = commandPool, .commandBufferCount = 1};
    chk(vkAllocateCommandBuffers(device, &cbOneTimeAI, &cbOneTime));

// Record the commands to the buffer
#pragma region
    VkCommandBufferBeginInfo cbOneTimeBI{.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));

    // Set image layout to one which allows transferrring data to it
    // (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    VkImageMemoryBarrier2 barrierTexImage{
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = textures[i].image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = ktxTexture->numLevels,
                .layerCount = 1,
            },
    };
    VkDependencyInfo barrierTexInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrierTexImage,
    };
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
    // transfer the data.
    std::vector<VkBufferImageCopy> copyRegions{};
    for (auto j = 0; j < ktxTexture->numLevels; j++) {
      ktx_size_t mipOffset{0};
      KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
      copyRegions.push_back({
          .bufferOffset = mipOffset,
          .imageSubresource{
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
          .imageExtent{.width = ktxTexture->baseWidth >> j,
                       .height = ktxTexture->baseHeight >> j,
                       .depth = 1},
      });
    }
    vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, textures[i].image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

    // Set image layout to one that allows reading data from it (VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
    VkImageMemoryBarrier2 barrierTexRead{
        .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        .image = textures[i].image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = ktxTexture->numLevels,
                .layerCount = 1,
            },
    };
    barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);

    chk(vkEndCommandBuffer(cbOneTime));

    // Submit the commad buffer
    VkSubmitInfo oneTimeSI{.commandBufferCount = 1, .pCommandBuffers = &cbOneTime};
    chk(vkQueueSubmit(queue, 1, &oneTimeSI, fenceOneTime));
    chk(vkWaitForFences(device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device, fenceOneTime, nullptr);
    vmaDestroyBuffer(allocator, imgSrcBuffer, imgSrcAllocation);
#pragma endregion

    // Create a sampler for the shader
    VkSamplerCreateInfo samplerCI{
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 8.0f,
        .maxLod = (float)ktxTexture->numLevels,
    };
    chk(vkCreateSampler(device, &samplerCI, nullptr, &textures[i].sampler));

    ktxTexture_Destroy(ktxTexture);

    textureDescriptors.push_back({
        .sampler = textures[i].sampler,
        .imageView = textures[i].view,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
    });
  }
#pragma endregion

// Use descriptors to pass information to the GPU about how the images look like and how they are
// accessed
#pragma region
  // Create Descriptor set layout. One sampler per image.
  VkDescriptorBindingFlags descVariableFlag{VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
  VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
      .bindingCount = 1,
      .pBindingFlags = &descVariableFlag,
  };
  VkDescriptorSetLayoutBinding descLayoutBindingTex{
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(textures.size()),
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
      .pNext = &descBindingFlags,
      .bindingCount = 1,
      .pBindings = &descLayoutBindingTex,
  };
  chk(vkCreateDescriptorSetLayout(device, &descLayoutTexCI, nullptr, &descriptorSetLayoutTex));

  // Create descriptor pool
  VkDescriptorPoolSize poolSize{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(textures.size()),
  };
  VkDescriptorPoolCreateInfo descPoolCI{.maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &poolSize};
  chk(vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool));

  // Allocate the descriptor set. Only one is needed because it combines images and samplers
  uint32_t variableDescCount{static_cast<uint32_t>(textures.size())};
  VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
      .descriptorSetCount = 1,
      .pDescriptorCounts = &variableDescCount,
  };
  VkDescriptorSetAllocateInfo texDescSetAlloc{
      .pNext = &variableDescCountAI,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &descriptorSetLayoutTex,
  };
  chk(vkAllocateDescriptorSets(device, &texDescSetAlloc, &descriptorSetTex));

  VkWriteDescriptorSet writeDescSet{
      .dstSet = descriptorSetTex,
      .dstBinding = 0,
      .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = textureDescriptors.data(),
  };
  vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
#pragma endregion

// Load the shader, compile it and pass it to the GPU
#pragma region
  slang::createGlobalSession(slangGlobalSession.writeRef());
  auto slangTargets{
      std::to_array<slang::TargetDesc>({{
          .format{SLANG_SPIRV},
          .profile{slangGlobalSession->findProfile("spirv_1_4")},
      }}),
  };

  auto slangOptions{
      std::to_array<slang::CompilerOptionEntry>({{
          slang::CompilerOptionName::EmitSpirvDirectly,
          {slang::CompilerOptionValueKind::Int, 1},
      }}),
  };
  slang::SessionDesc slangSessionDesc{
      .targets{slangTargets.data()},
      .targetCount{SlangInt(slangTargets.size())},
      // Use a layout compatible with the GLM library
      .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
      .compilerOptionEntries{slangOptions.data()},
      .compilerOptionEntryCount{uint32_t(slangOptions.size())},
  };
  Slang::ComPtr<slang::ISession> slangSession;
  slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

  Slang::ComPtr<slang::IModule> slangModule{
      slangSession->loadModuleFromSource("triangle", "assets/shader.slang", nullptr, nullptr),
  };
  Slang::ComPtr<ISlangBlob> spirv;
  slangModule->getTargetCode(0, spirv.writeRef());

  VkShaderModuleCreateInfo shaderModuleCI{
      .codeSize = spirv->getBufferSize(),
      .pCode = (uint32_t*)spirv->getBufferPointer(),
  };
  VkShaderModule shaderModule{};
  chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));
#pragma endregion

// PIPELINE
#pragma region
  // Pipeline layout
  VkPushConstantRange pushConstantRange{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .size = sizeof(VkDeviceAddress),
  };
  VkPipelineLayoutCreateInfo pipelineLayoutCI{
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayoutTex,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstantRange,
  };
  chk(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

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
      // Normal
      {.location = 1,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, normal)},
      // UV
      {.location = 2,
       .binding = 0,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(Vertex, uv)},
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
  VkPipelineDepthStencilStateCreateInfo depthStencilState{
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL};
  VkPipelineColorBlendAttachmentState blendAttachment{.colorWriteMask = 0xF};
  VkPipelineColorBlendStateCreateInfo colorBlendState{
      .attachmentCount = 1,
      .pAttachments = &blendAttachment,
  };
  VkPipelineRenderingCreateInfo renderingCI{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &imageFormat,
      .depthAttachmentFormat = depthFormat,
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
      .pDepthStencilState = &depthStencilState,
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
    shaderData.projection = glm::perspective(
        glm::radians(45.0f), (float)windowSize.x / (float)windowSize.y, 0.1f, 32.0f);
    shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
    for (auto i = 0; i < 3; i++) {
      auto instancePos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
      shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos);
    }
    memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData,
           sizeof(ShaderData));
    // Build command buffer
    auto cb = commandBuffers[frameIndex];
    chk(vkResetCommandBuffer(cb, 0));
    VkCommandBufferBeginInfo cbBI{
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));
    std::array<VkImageMemoryBarrier2, 2> outputBarriers{
        VkImageMemoryBarrier2{
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = swapchainImages[imageIndex],
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
        },
        VkImageMemoryBarrier2{
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = depthImage,
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        },
    };
    VkDependencyInfo barrierDependencyInfo{
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = outputBarriers.data(),
    };
    vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
    VkRenderingAttachmentInfo colorAttachmentInfo{
        .imageView = swapchainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{.color{0.0f, 0.0f, 0.0f, 1.0f}},
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
        .imageView = depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f, 0}},
    };
    VkRenderingInfo renderingInfo{
        .renderArea{.extent{.width = static_cast<uint32_t>(windowSize.x),
                            .height = static_cast<uint32_t>(windowSize.y)}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo};
    vkCmdBeginRendering(cb, &renderingInfo);
    VkViewport vp{.width = static_cast<float>(windowSize.x),
                  .height = static_cast<float>(windowSize.y),
                  .minDepth = 0.0f,
                  .maxDepth = 1.0f};
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{.extent{.width = static_cast<uint32_t>(windowSize.x),
                             .height = static_cast<uint32_t>(windowSize.y)}};
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetScissor(cb, 0, 1, &scissor);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &descriptorSetTex, 0, nullptr);
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
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
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
    frameIndex = (frameIndex + 1) % maxFramesInFlight;
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
      vmaDestroyImage(allocator, depthImage, depthImageAllocation);
      vkDestroyImageView(device, depthImageView, nullptr);
      depthImageCI.extent = {.width = static_cast<uint32_t>(windowSize.x),
                             .height = static_cast<uint32_t>(windowSize.y),
                             .depth = 1};
      VmaAllocationCreateInfo allocCI{.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                                      .usage = VMA_MEMORY_USAGE_AUTO};
      chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation,
                         nullptr));
      VkImageViewCreateInfo viewCI{
          .image = depthImage,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = depthFormat,
          .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                               .levelCount = 1,
                               .layerCount = 1},
      };
      chk(vkCreateImageView(device, &viewCI, nullptr, &depthImageView));
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
  vmaDestroyImage(allocator, depthImage, depthImageAllocation);
  vkDestroyImageView(device, depthImageView, nullptr);
  for (auto i = 0; i < swapchainImageViews.size(); i++) {
    vkDestroyImageView(device, swapchainImageViews[i], nullptr);
  }
  vmaDestroyBuffer(allocator, vBuffer, vBufferAllocation);
  for (auto i = 0; i < textures.size(); i++) {
    vkDestroyImageView(device, textures[i].view, nullptr);
    vkDestroySampler(device, textures[i].sampler, nullptr);
    vmaDestroyImage(allocator, textures[i].image, textures[i].allocation);
  }
  vkDestroyDescriptorSetLayout(device, descriptorSetLayoutTex, nullptr);
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