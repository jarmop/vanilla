#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_FRAMES_IN_FLIGHT 2

void CHECK(int x) {
  if ((x) != VK_SUCCESS) {
    printf("Vulkan error: %d\n", x);
    exit(1);
  }
}

GLFWwindow* window;

/* Vulkan core objects */
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;

VkQueue graphicsQueue;
VkQueue presentQueue;

uint32_t graphicsFamily = -1;
uint32_t presentFamily = -1;

/* Swapchain */
VkSwapchainKHR swapchain;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;

VkImage* swapchainImages;
uint32_t swapchainImageCount;

VkImageView* swapchainImageViews;

/* Pipeline */
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

/* Framebuffers */
VkFramebuffer* framebuffers;

/* Command */
VkCommandPool commandPool;
VkCommandBuffer* commandBuffers;

/* Sync */
VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

size_t currentFrame = 0;

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

void initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", NULL, NULL);
}

/* Instance */
void createInstance() {
  VkApplicationInfo app = {.apiVersion = VK_API_VERSION_1_3};

  uint32_t count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);

  VkInstanceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app,
    .enabledExtensionCount = count,
    .ppEnabledExtensionNames = extensions,
  };

  CHECK(vkCreateInstance(&info, NULL, &instance));
}

/* Surface */
void createSurface() { CHECK(glfwCreateWindowSurface(instance, window, NULL, &surface)); }

/* Device selection */
int isDeviceSuitable(VkPhysicalDevice dev) {
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, NULL);
  VkQueueFamilyProperties props[count];
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, props);

  for (uint32_t i = 0; i < count; i++) {
    if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;

    VkBool32 presentSupport = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
    if (presentSupport) presentFamily = i;
  }

  return graphicsFamily != -1 && presentFamily != -1;
}

void pickPhysicalDevice() {
  uint32_t count;
  vkEnumeratePhysicalDevices(instance, &count, NULL);
  VkPhysicalDevice devices[count];
  vkEnumeratePhysicalDevices(instance, &count, devices);

  for (uint32_t i = 0; i < count; i++) {
    if (isDeviceSuitable(devices[i])) {
      physicalDevice = devices[i];
      break;
    }
  }
}

/* Logical device */
void createLogicalDevice() {
  float priority = 1.0f;

  VkDeviceQueueCreateInfo queues[2];
  uint32_t unique[] = {graphicsFamily, presentFamily};

  for (int i = 0; i < 2; i++) {
    queues[i] = (VkDeviceQueueCreateInfo){
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = unique[i],
      .queueCount = 1,
      .pQueuePriorities = &priority,
    };
  }

  const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = (graphicsFamily == presentFamily) ? 1 : 2,
    .pQueueCreateInfos = queues,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = extensions,
  };

  CHECK(vkCreateDevice(physicalDevice, &info, NULL, &device));

  vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
  vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
}

static VkExtent2D choose_extent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR* caps) {
  if (caps->currentExtent.width != UINT32_MAX) return caps->currentExtent;

  int w, h;
  glfwGetFramebufferSize(window, &w, &h);
  VkExtent2D e = {(uint32_t)w, (uint32_t)h};

  if (e.width < caps->minImageExtent.width) e.width = caps->minImageExtent.width;
  if (e.width > caps->maxImageExtent.width) e.width = caps->maxImageExtent.width;
  if (e.height < caps->minImageExtent.height) e.height = caps->minImageExtent.height;
  if (e.height > caps->maxImageExtent.height) e.height = caps->maxImageExtent.height;
  return e;
}

/* Swapchain */
void createSwapchain() {
  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

  swapchainExtent = choose_extent(window, &caps);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
  VkSurfaceFormatKHR formats[formatCount];
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

  VkSurfaceFormatKHR format = formats[0];
  swapchainImageFormat = format.format;

  VkSwapchainCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = caps.minImageCount + 1,
    .imageFormat = format.format,
    .imageColorSpace = format.colorSpace,
    .imageExtent = swapchainExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .preTransform = caps.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
  };

  CHECK(vkCreateSwapchainKHR(device, &info, NULL, &swapchain));

  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
  swapchainImages = malloc(sizeof(VkImage) * swapchainImageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);
}

/* Image views */
void createImageViews() {
  swapchainImageViews = malloc(sizeof(VkImageView) * swapchainImageCount);

  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageViewCreateInfo view = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = swapchainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = swapchainImageFormat,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
    CHECK(vkCreateImageView(device, &view, NULL, &swapchainImageViews[i]));
  }
}

/* Render pass */
void createRenderPass() {
  VkAttachmentDescription color = {
    .format = swapchainImageFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference ref = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &ref,
  };

  VkRenderPassCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &color,
    .subpassCount = 1,
    .pSubpasses = &subpass,
  };

  CHECK(vkCreateRenderPass(device, &info, NULL, &renderPass));
}

/* Shader module */
VkShaderModule createShaderModule(const char* file) {
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

/* Pipeline */
void createPipeline() {
  VkShaderModule vert = createShaderModule("shaders/vert.spv");
  VkShaderModule frag = createShaderModule("shaders/frag.spv");

  VkPipelineShaderStageCreateInfo stages[2] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag,
      .pName = "main",
    }};

  VkPipelineVertexInputStateCreateInfo vertex = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo input = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

  VkViewport viewport = {
    .width = (float)swapchainExtent.width,
    .height = (float)swapchainExtent.height,
    .maxDepth = 1.0f,
  };

  VkRect2D scissor = {{0, 0}, swapchainExtent};

  VkPipelineViewportStateCreateInfo viewportState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo raster = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
  };

  VkPipelineMultisampleStateCreateInfo msaa = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineColorBlendAttachmentState blendAttach = {.colorWriteMask = 0xF};

  VkPipelineColorBlendStateCreateInfo blend = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &blendAttach,
  };

  VkPipelineLayoutCreateInfo layoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  CHECK(vkCreatePipelineLayout(device, &layoutInfo, NULL, &pipelineLayout));

  VkGraphicsPipelineCreateInfo pipe = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = stages,
    .pVertexInputState = &vertex,
    .pInputAssemblyState = &input,
    .pViewportState = &viewportState,
    .pRasterizationState = &raster,
    .pMultisampleState = &msaa,
    .pColorBlendState = &blend,
    .layout = pipelineLayout,
    .renderPass = renderPass,
    .subpass = 0,
  };

  CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, NULL, &graphicsPipeline));

  vkDestroyShaderModule(device, vert, NULL);
  vkDestroyShaderModule(device, frag, NULL);
}

/* Framebuffers */
void createFramebuffers() {
  framebuffers = malloc(sizeof(VkFramebuffer) * swapchainImageCount);

  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageView attachments[] = {swapchainImageViews[i]};

    VkFramebufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderPass,
      .attachmentCount = 1,
      .pAttachments = attachments,
      .width = swapchainExtent.width,
      .height = swapchainExtent.height,
      .layers = 1,
    };

    CHECK(vkCreateFramebuffer(device, &info, NULL, &framebuffers[i]));
  }
}

/* Command buffers */
void createCommandBuffers() {
  VkCommandPoolCreateInfo pool = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .queueFamilyIndex = graphicsFamily,
  };

  CHECK(vkCreateCommandPool(device, &pool, NULL, &commandPool));

  commandBuffers = malloc(sizeof(VkCommandBuffer) * swapchainImageCount);

  VkCommandBufferAllocateInfo alloc = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = swapchainImageCount,
  };

  CHECK(vkAllocateCommandBuffers(device, &alloc, commandBuffers));

  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    vkBeginCommandBuffer(commandBuffers[i], &begin);

    VkClearValue clear = {{{0.1f, 0.1f, 0.1f, 1.0f}}};

    VkRenderPassBeginInfo rp = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderPass,
      .framebuffer = framebuffers[i],
      .renderArea = {{0, 0}, swapchainExtent},
      .clearValueCount = 1,
      .pClearValues = &clear,
    };

    vkCmdBeginRenderPass(commandBuffers[i], &rp, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffers[i]);

    vkEndCommandBuffer(commandBuffers[i]);
  }
}

/* Sync */
void createSync() {
  VkSemaphoreCreateInfo sem = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fence = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CHECK(vkCreateSemaphore(device, &sem, NULL, &imageAvailableSemaphores[i]));
    CHECK(vkCreateSemaphore(device, &sem, NULL, &renderFinishedSemaphores[i]));
    CHECK(vkCreateFence(device, &fence, NULL, &inFlightFences[i]));
  }
}

/* Draw */
void drawFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                        VK_NULL_HANDLE, &imageIndex);

  VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};

  VkSemaphore waitSem[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags stage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = waitSem;
  submit.pWaitDstStageMask = stage;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commandBuffers[imageIndex];

  VkSemaphore signalSem[] = {renderFinishedSemaphores[currentFrame]};
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = signalSem;

  vkQueueSubmit(graphicsQueue, 1, &submit, inFlightFences[currentFrame]);

  VkPresentInfoKHR present = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signalSem,
    .swapchainCount = 1,
    .pSwapchains = &swapchain,
    .pImageIndices = &imageIndex,
  };

  vkQueuePresentKHR(presentQueue, &present);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/* Main loop */
void loop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device);
}

/* Cleanup */
void cleanup() {
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
    vkDestroyFence(device, inFlightFences[i], NULL);
  }

  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    vkDestroyFramebuffer(device, framebuffers[i], NULL);
    vkDestroyImageView(device, swapchainImageViews[i], NULL);
  }

  vkDestroyPipeline(device, graphicsPipeline, NULL);
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  vkDestroySwapchainKHR(device, swapchain, NULL);
  vkDestroyCommandPool(device, commandPool, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  vkDestroyInstance(instance, NULL);

  glfwDestroyWindow(window);
  glfwTerminate();
}

/* Entry */
int main() {
  initWindow();
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapchain();
  createImageViews();
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandBuffers();
  createSync();

  loop();
  cleanup();
  return 0;
}