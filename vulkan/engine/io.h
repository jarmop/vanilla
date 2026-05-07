#ifndef H_IO
#define H_IO

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#include "types.h"

bool framebufferResized = false;

const float MOUSE_SENSITIVITY = 0.1;
const float MAX_FOV = 45.0f;

Camera camera = {
  {0.0f, 0.0f, 2.0f},   // pos
  {0.0f, 0.0f, -1.0f},  // front
  {1.0f, 0.0f, 0.0f},   // right
  -90.0,                // yaw
  0.0,                  // pitch
  2.5,                  // speed
  MAX_FOV               // fov
};

vec3 worldUp = {0.0f, 1.0f, 0.0f};

float prevMouseX, prevMouseY;
bool mouseRightPressed = false;
bool firstMouse = true;
float previousFrameTime = 0.0;  // Time of the previous frame

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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
  (void)window;
  (void)mods;
  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      mouseRightPressed = true;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
      mouseRightPressed = false;
      firstMouse = true;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
  }
}

void updateCamera(Camera* camera) {
  camera->front[0] = cos(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
  camera->front[1] = sin(glm_rad(camera->pitch));
  camera->front[2] = sin(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
  glm_normalize(camera->front);
  glm_cross(camera->front, worldUp, camera->right);
  glm_normalize(camera->right);
}

void mouse_callback(GLFWwindow* window, double mouseX, double mouseY) {
  (void)window;
  if (!mouseRightPressed) {
    return;
  }
  // fprintf(stderr, "x: %d, y: %d\n", (int)xpos, (int)ypos);

  Camera* camera = glfwGetWindowUserPointer(window);

  if (firstMouse) {
    prevMouseX = mouseX;
    prevMouseY = mouseY;
    firstMouse = false;
  }

  camera->yaw += (mouseX - prevMouseX) * MOUSE_SENSITIVITY;
  camera->pitch -= (mouseY - prevMouseY) * MOUSE_SENSITIVITY;
  prevMouseX = mouseX;
  prevMouseY = mouseY;

  if (camera->pitch > 89.0) {
    camera->pitch = 89.0;
  } else if (camera->pitch < -89.0) {
    camera->pitch = -89.0;
  }

  updateCamera(camera);
}

/**
 * GLFW input getters like glfwGetKey return the last state for the specified
 * key, ignoring GLFW_REPEAT as it's the same as GLFW_PRESS in this context, so
 * they only return GLFW_PRESS or GLFW_KEY.
 */
void handleCameraMovementKeys(GLFWwindow* window, Camera* camera) {
  float currentTime = glfwGetTime();
  const float cameraMovement = camera->speed * (currentTime - previousFrameTime);
  previousFrameTime = currentTime;

  // WASD
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    glm_vec3_muladds(camera->front, cameraMovement, camera->pos);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    glm_vec3_mulsubs(camera->front, cameraMovement, camera->pos);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    glm_vec3_mulsubs(camera->right, cameraMovement, camera->pos);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    glm_vec3_muladds(camera->right, cameraMovement, camera->pos);
  }

  // Elevation
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
    glm_vec3_muladds(worldUp, cameraMovement, camera->pos);
  }
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    glm_vec3_mulsubs(worldUp, cameraMovement, camera->pos);
  }
}

void initIo(GLFWwindow* window, Engine* ng) {
  ng->framebufferResized = &framebufferResized;
  ng->camera = &camera;
  ng->worldUp = &worldUp;

  glfwSetWindowUserPointer(window, &camera);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  updateCamera(&camera);
}

#endif