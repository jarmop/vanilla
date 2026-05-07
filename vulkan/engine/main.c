#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "init.h"
#include "io.h"
#include "render.h"
#include "types.h"

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Tutorial", NULL, NULL);

  Engine* ng = init(window);

  initIo(window, ng);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    handleCameraMovementKeys(window, ng->camera);
    updateUniformBuffer(*ng, *ng->camera, *ng->worldUp);

    drawFrame(ng->device, &ng->syncObjects, &ng->swapchain, ng->commandBuffers,
              ng->graphicsPipeline, ng->pipelineLayout, ng->descriptorSets, ng->vertexBuffer,
              ng->indexBuffer, ng->queue);

    if (*ng->framebufferResized) {
      *ng->framebufferResized = false;

      // If window was minimized, Wait until it's expanded again
      int width = 0, height = 0;
      glfwGetFramebufferSize(window, &width, &height);
      while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
      }

      handleWindowResize(ng);
    }
  }

  return 0;
}