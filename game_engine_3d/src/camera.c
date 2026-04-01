#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <cglm/cglm.h>

#include "../glad/glad.h"
#include <GLFW/glfw3.h>

#include "../lib/get_shader_program.h"
#include "../lib/stb_image.h"
#include "../lib/util.h"

const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;
const float MOUSE_SENSITIVITY = 0.1;
const float MAX_FOV = 45.0;

struct camera {
    vec3 pos;
    vec3 front;
    vec3 right;
    float yaw;
    float pitch;
    float speed;
    float fov;
} camera = {
    {0.0, 0.0, 3.0}, // pos
    {0.0, 0.0, -1.0}, // front
    {1.0, 0.0, 0.0},  // right
    -90.0, // yaw
    0.0, // pitch
    2.5, // speed
    MAX_FOV // fov
};

vec3 worldUp = {0.0, 1.0, 0.0};

float prevMouseX, prevMouseY;
bool mouseRightPressed = false;
bool firstMouse = true;
float previousFrameTime = 0.0; // Time of the previous frame

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void) window;
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
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

void mouse_callback(GLFWwindow* window, double mouseX, double mouseY) {
    (void) window;
    if (!mouseRightPressed) {
        return;
    }
    // fprintf(stderr, "x: %d, y: %d\n", (int)xpos, (int)ypos);

    if (firstMouse) {
        prevMouseX = mouseX;
        prevMouseY = mouseY;
        firstMouse = false;
    }

    camera.yaw += (mouseX - prevMouseX) * MOUSE_SENSITIVITY;
    camera.pitch -= (mouseY - prevMouseY) * MOUSE_SENSITIVITY;
    prevMouseX = mouseX;
    prevMouseY = mouseY;

    if (camera.pitch > 89.0) {
        camera.pitch = 89.0;
    } else if (camera.pitch < -89.0) {
        camera.pitch = -89.0;
    }

    camera.front[0] = cos(glm_rad(camera.yaw)) * cos(glm_rad(camera.pitch));
    camera.front[1] = sin(glm_rad(camera.pitch));
    camera.front[2] = sin(glm_rad(camera.yaw)) * cos(glm_rad(camera.pitch));
    glm_normalize(camera.front);
    glm_cross(camera.front, worldUp, camera.right);
    glm_normalize(camera.right);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // fprintf(stderr, "%d\n", (int)fov);
    (void)window; (void)xoffset;
    camera.fov -= (float)yoffset;
    if (camera.fov < 1.0f)
        camera.fov = 1.0f;
    if (camera.fov > MAX_FOV)
        camera.fov = MAX_FOV; 
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void) scancode; (void) mods;
    // fprintf(stderr, "%d - %d\n", key, action);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}


/**
 * GLFW input getters like glfwGetKey return the last state for the specified 
 * key, ignoring GLFW_REPEAT as it's the same as GLFW_PRESS in this context, so 
 * they only return GLFW_PRESS or GLFW_KEY.
 */
void handle_camera_movement_keys(GLFWwindow *window) {
    float currentTime = glfwGetTime();
    const float cameraMovement = camera.speed * (currentTime - previousFrameTime);
    previousFrameTime = currentTime;
    
    // WSAD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        glm_vec3_muladds(camera.front, cameraMovement, camera.pos);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        glm_vec3_mulsubs(camera.front, cameraMovement, camera.pos);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm_vec3_mulsubs(camera.right, cameraMovement, camera.pos);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm_vec3_muladds(camera.right, cameraMovement, camera.pos);
    }

    // Elevation
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        glm_vec3_muladds(worldUp, cameraMovement, camera.pos);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        glm_vec3_mulsubs(worldUp, cameraMovement, camera.pos);
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return 1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Render elements at the front over the ones in the back
    glEnable(GL_DEPTH_TEST);

    GLuint shaderProgram = get_shader_program("src/shader.vs", "src/shader.fs");
    glUseProgram(shaderProgram);

    // Vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex buffer object
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // glVertexAttribPointer args --> index, size, type, normalized, stride, pointer
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
    // glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint texture;    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // Tell shader that texSampler should use texture unit 0

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load("assets/container.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        fprintf(stderr, "Failed to load texture\n");
    }
    stbi_image_free(data);

    unsigned int cubeCount = 10;
    vec3 cubePositions[] = {
        { 0.0f,  0.0f,  0.0f},
        { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f},
        {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f},
        {-1.7f,  3.0f, -7.5f},
        { 1.3f, -2.0f, -2.5f},
        { 1.5f,  2.0f, -2.5f},
        { 1.5f,  0.2f, -1.5f},
        {-1.3f,  1.0f, -1.5f}
    };

    // bind Texture
    glBindTexture(GL_TEXTURE_2D, texture);

    // Render loop. Each iteration renders a new frame.
    while(!glfwWindowShouldClose(window)) {
        glClearColor(0.53f, 0.18f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 view;
        vec3 cameraPosFront;
        glm_vec3_add(camera.pos, camera.front, cameraPosFront);
        glm_lookat(camera.pos, cameraPosFront, worldUp, view);
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (GLfloat *)view);

        mat4 projection;
        glm_perspective(glm_rad(camera.fov), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f, projection);
        int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, (GLfloat *)projection);

        for (unsigned int i = 0; i < cubeCount; i++) {
            mat4 model;
            glm_mat4_identity(model);
            glm_translate(model, cubePositions[i]);
            // glm_rotate(model, (float)glfwGetTime() + i * 20, (vec3){1.0f, 0.3f, 0.5f});
            // glm_rotate(model, glm_rad(20.0f * i), (vec3){1.0f, 0.3f, 0.5f});
            int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (GLfloat *)model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);

        // Check input events and trigger the appropriate callback functions
        glfwPollEvents();

        handle_camera_movement_keys(window);
    }

    return 0;
}