#ifndef H_INIT_MODELS
#define H_INIT_MODELS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "types.h"

void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer,
                VkBuffer dstBuffer, VkDeviceSize bufferSize) {
  VkCommandBuffer* comCopyBuffers = createCommandBuffers(device, commandPool, 1);
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
  free(comCopyBuffers);
}

void createStagedBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                        VkQueue queue, VkDeviceSize bufferSize, const void* data,
                        VkBufferUsageFlags usageFlags, VkBuffer* buffer) {
  // STAGING BUFFER
  VkBuffer bufferStaging;
  VkDeviceMemory bufferMemoryStaging;
  createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &bufferStaging, &bufferMemoryStaging);

  void* tempData;
  vkMapMemory(device, bufferMemoryStaging, 0, bufferSize, 0, &tempData);
  memcpy(tempData, data, (size_t)bufferSize);
  vkUnmapMemory(device, bufferMemoryStaging);

  // FINAL BUFFER
  VkDeviceMemory bufferMemory;
  createBuffer(device, physicalDevice, bufferSize, usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               //  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, &bufferMemory);

  // Copy staging to final
  copyBuffer(device, commandPool, queue, bufferStaging, *buffer, bufferSize);
}

void createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                        VkQueue queue, VkBuffer* vertexBuffer) {
  // clang-format off
  const Vertex vertices[] = {
    {{-0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
  };
  // clang-format on
  VkDeviceSize bufferSize = sizeof(vertices);

  createStagedBuffer(device, physicalDevice, commandPool, queue, bufferSize, vertices,
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer);
}

void createIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                       VkQueue queue, VkBuffer* indexBuffer) {
  const uint32_t indices[] = {0, 1, 2, 2, 3, 0};
  VkDeviceSize bufferSize = sizeof(indices);

  createStagedBuffer(device, physicalDevice, commandPool, queue, bufferSize, indices,
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer);
}

#endif