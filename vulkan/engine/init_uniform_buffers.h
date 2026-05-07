#ifndef H_INIT_UNIFORM_BUFFERS
#define H_INIT_UNIFORM_BUFFERS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "init_buffer.h"
#include "types.h"

VkDescriptorPoolCreateInfo poolCI = {
  .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
  .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
  .maxSets = MAX_FRAMES_IN_FLIGHT,
  .poolSizeCount = 1,
  .pPoolSizes =
    &(VkDescriptorPoolSize){
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    },
};

VkDescriptorSetAllocateInfo descriptorSetAI = {
  .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
  .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
};

VkDescriptorBufferInfo descBufInfo = {
  .offset = 0,
  .range = sizeof(UniformBufferObject),
};
VkWriteDescriptorSet descriptorWrite = {
  .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
  .dstBinding = 0,
  .dstArrayElement = 0,
  .descriptorCount = 1,
  .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
};

void createDescriptorSets(VkDevice device, VkBuffer* uniformBuffers,
                          VkDescriptorSetLayout descriptorSetLayout,
                          VkDescriptorSet* descriptorSets) {
  vkCreateDescriptorPool(device, &poolCI, NULL, &descriptorSetAI.descriptorPool);
  VkDescriptorSetLayout layouts[] = {descriptorSetLayout, descriptorSetLayout};
  descriptorSetAI.pSetLayouts = layouts;
  vkAllocateDescriptorSets(device, &descriptorSetAI, descriptorSets);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    descBufInfo.buffer = uniformBuffers[i];
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.pBufferInfo = &descBufInfo;
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
  }
}

void createUniformBuffers(VkDevice device, VkPhysicalDevice physicalDevice,
                          void** uniformBuffersMapped, VkBuffer* uniformBuffers) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    VkBuffer buffer = {0};
    VkDeviceMemory bufferMem = {0};
    createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &buffer, &bufferMem);
    uniformBuffers[i] = buffer;
    vkMapMemory(device, bufferMem, 0, bufferSize, 0, &uniformBuffersMapped[i]);
  }
}

VkDescriptorSetLayoutCreateInfo layoutCI = {
  .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
  .bindingCount = 1,
  .pBindings =
    &(VkDescriptorSetLayoutBinding){
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_VERTEX_BIT,
    },
};

void initUniformBuffers(VkPhysicalDevice* physicalDevice, VkDevice* device,
                        void** uniformBuffersMapped, VkDescriptorSetLayout* descriptorSetLayout,
                        VkDescriptorSet* descriptorSets) {
  VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
  createUniformBuffers(*device, *physicalDevice, uniformBuffersMapped, uniformBuffers);
  vkCreateDescriptorSetLayout(*device, &layoutCI, NULL, descriptorSetLayout);
  createDescriptorSets(*device, uniformBuffers, *descriptorSetLayout, descriptorSets);
}

#endif