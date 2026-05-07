#ifndef NG_RENDER
#define NG_RENDER

#include <cglm/cglm.h>

#include "types.h"

size_t currentFrame = 0;

void recordCommandBuffers(VkCommandBuffer commandBuffer, VkPipeline graphicsPipeline,
                          VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSets,
                          VkBuffer vertexBuffer, VkBuffer indexBuffer, Swapchain* swapchain,
                          uint32_t imageIndex) {
  // Bunch of stuff needs to be based on the imageIndex rather than frame. So even if there is only
  // one command buffer per "frame in flight" (2) the commands are recreated on every image (4)
  VkCommandBufferBeginInfo begin = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  vkBeginCommandBuffer(commandBuffer, &begin);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
  // Viewport y axis is inverted to match OpenGL and glm_lookat:
  // viewport y: 0 --> swapchain.extent.height
  // viewport height: swapchain.extent.height --> -swapchain.extent.height
  vkCmdSetViewport(commandBuffer, 0, 1,
                   (VkViewport[]){{
                     .y = (float)swapchain->extent.height,
                     .width = (float)swapchain->extent.width,
                     .height = -(float)swapchain->extent.height,
                     .maxDepth = 1.0f,
                   }});
  vkCmdSetScissor(commandBuffer, 0, 1, (VkRect2D[]){{{0, 0}, swapchain->extent}});
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[currentFrame], 0, NULL);

  VkDeviceSize vertexOffset = 0;
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexOffset);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
    .image = swapchain->images[imageIndex],
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
    .imageView = swapchain->imageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .clearValue = clearValue,
  };
  VkRenderingInfo renderingInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea = {.extent = swapchain->extent},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentInfo,
  };
  vkCmdBeginRendering(commandBuffer, &renderingInfo);
  vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
  vkCmdEndRendering(commandBuffer);

  // barrier.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

  vkEndCommandBuffer(commandBuffer);
}

// void updateUniformBuffer(Swapchain* swapchain, void** uniformBuffersMapped) {
void ngUpdateUniformBuffer(Engine ng, Camera camera, vec3 worldUp) {
  UniformBufferObject ubo = {0};
  glm_mat4_identity(ubo.model);
  vec3 cameraPosFront;
  glm_vec3_add(camera.pos, camera.front, cameraPosFront);
  glm_lookat(camera.pos, cameraPosFront, worldUp, ubo.view);
  glm_perspective(glm_rad(camera.fov),
                  (float)ng.swapchain.extent.width / (float)ng.swapchain.extent.height, 0.1f,
                  100.0f, ubo.proj);
  memcpy(ng.uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void ngDrawFrame(VkDevice device, SyncObjects* syncObjects, Swapchain* swapchain,
                 VkCommandBuffer* commandBuffers, VkPipeline graphicsPipeline,
                 VkPipelineLayout pipelineLayout, VkDescriptorSet* descriptorSets,
                 VkBuffer vertexBuffer, VkBuffer indexBuffer, VkQueue queue) {
  vkWaitForFences(device, 1, &syncObjects->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &syncObjects->inFlightFences[currentFrame]);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapchain->handle, UINT64_MAX,
                        syncObjects->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                        &imageIndex);

  VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
  // vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
  recordCommandBuffers(commandBuffer, graphicsPipeline, pipelineLayout, descriptorSets,
                       vertexBuffer, indexBuffer, swapchain, imageIndex);

  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = (VkSemaphore[]){syncObjects->imageAvailableSemaphores[currentFrame]},
    .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = (VkSemaphore[]){syncObjects->renderFinishedSemaphores[imageIndex]},
  };
  vkQueueSubmit(queue, 1, &submitInfo, syncObjects->inFlightFences[currentFrame]);

  VkPresentInfoKHR presentInfo = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = (VkSemaphore[]){syncObjects->renderFinishedSemaphores[imageIndex]},
    .swapchainCount = 1,
    .pSwapchains = &swapchain->handle,
    .pImageIndices = &imageIndex,
  };
  vkQueuePresentKHR(queue, &presentInfo);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

#endif