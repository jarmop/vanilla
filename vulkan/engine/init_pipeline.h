#ifndef H_INIT_PIPELINE
#define H_INIT_PIPELINE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "types.h"

VkGraphicsPipelineCreateInfo pipelineCI = {
  .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
  .stageCount = 2,
  .pVertexInputState =
    &(VkPipelineVertexInputStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]){{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      }},
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions =
        (VkVertexInputAttributeDescription[]){
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
        },
    },
  .pInputAssemblyState =
    &(VkPipelineInputAssemblyStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    },
  .pViewportState =
    &(VkPipelineViewportStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
    },
  .pRasterizationState =
    &(VkPipelineRasterizationStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
    },
  .pMultisampleState =
    &(VkPipelineMultisampleStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      // .sampleShadingEnable = VK_TRUE,
    },
  .pColorBlendState =
    &(VkPipelineColorBlendStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = (VkPipelineColorBlendAttachmentState[]){{
        // .colorWriteMask = 0xF,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      }},
    },
  .pDynamicState =
    &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = (VkDynamicState[]){VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
    },
};

VkPipelineRenderingCreateInfo renderingCI = {
  .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
  .colorAttachmentCount = 1,
};

VkPipelineShaderStageCreateInfo stageCIs[] = {
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .pName = "vertMain",
  },
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .pName = "fragMain",
  },
};

VkPipelineLayoutCreateInfo pipelineLayoutCI = {
  .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
  .setLayoutCount = 1,
  .pushConstantRangeCount = 0,
  .pPushConstantRanges = NULL,
};

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

VkShaderModule createShaderModule(const char* file, VkDevice device) {
  size_t size;
  char* code = readFile(file, &size);
  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = (uint32_t*)code,
  };
  VkShaderModule module;
  vkCreateShaderModule(device, &info, NULL, &module);
  free(code);
  return module;
}

void createPipeline(VkDevice device, Swapchain* swapchain,
                    VkDescriptorSetLayout descriptorSetLayout, VkPipelineLayout* pipelineLayout,
                    VkPipeline* graphicsPipeline) {
  renderingCI.pColorAttachmentFormats = (VkFormat[]){swapchain->imageFormat};

  VkShaderModule shaderModule = createShaderModule("shaders/out.spv", device);
  stageCIs[0].module = shaderModule;
  stageCIs[1].module = shaderModule;

  pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
  vkCreatePipelineLayout(device, &pipelineLayoutCI, NULL, pipelineLayout);

  pipelineCI.pNext = &renderingCI;
  pipelineCI.pStages = stageCIs;
  pipelineCI.layout = *pipelineLayout;
  vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, NULL, graphicsPipeline);

  vkDestroyShaderModule(device, shaderModule, NULL);
}

#endif