#ifndef H_INIT_BUFFER
#define H_INIT_BUFFER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>

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

VkCommandBuffer* createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count) {
  VkCommandBufferAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = count,
  };
  VkCommandBuffer* commandBuffers = malloc(sizeof(VkCommandBuffer) * info.commandBufferCount);
  vkAllocateCommandBuffers(device, &info, commandBuffers);
  return commandBuffers;
}

#endif