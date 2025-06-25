#include "Common.h"
#include "Engine/Camera.h"
#include "Engine/Chunk.h"
#include "Engine/InfiniteWorld.h"
#include "Frustum.h"
#include "Generation/Biomes.h"
#include "Generation/Noise.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <random>
unsigned int GLOBAL_SEED = 0;

#include <iostream>
/* TODO
    * - Implement proper error handling for OpenGL and ImGui initialization.
    * - Add more biomes and their specific properties.
    * - Optimize chunk rendering and loading.
    * - Implement a more sophisticated noise generation algorithm for terrain.
    * - Add lighting effects and shadows.
    * - Add Atlas texture support for voxel textures.
*/

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(vec3(0.0f, 50.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

/**
 * @brief Processes keyboard input for camera movement and window control.
 *
 * Updates camera position based on movement keys (W, A, S, D, SPACE, LEFT_SHIFT) and closes the window when Escape is pressed.
 */
void processInput(GLFWwindow* window) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(0, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(3, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.processKeyboard(4, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.processKeyboard(5, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

/**
 * @brief Handles mouse movement to update the camera's orientation.
 *
 * Updates the camera's yaw and pitch based on mouse movement, enabling first-person look controls. Initializes the last mouse position on the first event to prevent sudden jumps.
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed: y ranges bottom to top
    lastX = xpos;
    lastY = ypos;
    camera.processMouseMovement(xoffset, yoffset);
}

/**
 * @brief Adjusts the OpenGL viewport when the window is resized.
 *
 * Updates the rendering area to match the new window dimensions.
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

/**
 * @brief Renders a debug UI window displaying FPS, camera position, current chunk, and biome information.
 *
 * Shows real-time performance and player location details using ImGui, including the biome at the player's current coordinates.
 */
void renderUI(const Camera& camera, float fps) {
    ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z);
    ImGui::Text("Chunk: (%d, %d)", camera.getCurrentChunkCoord().x, camera.getCurrentChunkCoord().z);

    int playerX = static_cast<int>(camera.position.x);
    int playerZ = static_cast<int>(camera.position.z);
    Biome biome = selectBiome(playerX, playerZ, GLOBAL_SEED);
    ImGui::Text("Biome: %s", biome.name.c_str());

    ImGui::End();
}

/**
 * @brief Compiles and links a vertex and fragment shader into an OpenGL shader program.
 *
 * Compiles the provided vertex and fragment shader source code, checks for compilation and linking errors, and returns the resulting shader program ID.
 *
 * @param vertexSrc Source code for the vertex shader.
 * @param fragmentSrc Source code for the fragment shader.
 * @return GLuint The OpenGL shader program ID.
 */
GLuint compileShader(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compilation Failed:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compilation Failed:\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader Program Linking Failed:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

/**
 * @brief Performs ray-based voxel picking to find the first solid voxel intersected by the camera's view.
 *
 * Casts a ray from the camera position along its front direction up to a maximum distance, returning the coordinates of the first solid voxel hit and the approximate normal of the face intersected.
 *
 * @param outBlock Receives the coordinates of the intersected voxel if found.
 * @param outNormal Receives the normal vector of the intersected face, with components clamped to -1, 0, or 1.
 * @param maxDistance Maximum distance to search along the ray (default is 6.0).
 * @return true if a solid voxel is hit; false otherwise.
 */
bool pickVoxel(const Camera& camera, InfiniteWorld& world, glm::ivec3& outBlock, glm::ivec3& outNormal, float maxDistance = 6.0f){
    glm::vec3 origin = camera.position;
    glm::vec3 dir = glm::normalize(camera.front);
    glm::ivec3 lastBlock(-9999);
    for (float t = 0.0f; t < maxDistance; t += 0.05f) {
        glm::vec3 pos = origin + dir * t;
        glm::ivec3 block = glm::floor(pos);
        if (block != lastBlock) {
            if (world.isVoxelSolidAt(block.x, block.y, block.z)) {
                outBlock = block;
                // Approximate normal
                glm::vec3 prev = origin + dir * (t - 0.05f);
                glm::ivec3 prevBlock = glm::floor(prev);
                outNormal = prevBlock - block;
                // Clamp normal to -1/0/1
                outNormal.x = (outNormal.x > 0) ? 1 : (outNormal.x < 0 ? -1 : 0);
                outNormal.y = (outNormal.y > 0) ? 1 : (outNormal.y < 0 ? -1 : 0);
                outNormal.z = (outNormal.z > 0) ? 1 : (outNormal.z < 0 ? -1 : 0);
                return true;
            }
            lastBlock = block;
        }
    }
    return false;
}

/**
 * @brief Handles voxel interaction based on mouse input.
 *
 * Detects single left or right mouse button presses to remove or place voxels in the world at the targeted location. Left-click removes the selected voxel; right-click places a voxel adjacent to the targeted face if the space is empty.
 */
void processInteraction(GLFWwindow* window, const Camera& camera, InfiniteWorld& world) {
    static bool leftMousePressedLast = false;
    static bool rightMousePressedLast = false;

    bool leftMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool rightMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    glm::ivec3 block, normal;
    if (pickVoxel(camera, world, block, normal)) {
        // Remove block on left click (single press)
        if (leftMousePressed && !leftMousePressedLast) {
            world.setVoxel(block.x, block.y, block.z, AIR);
        }
        // Place block on right click (single press)
        if (rightMousePressed && !rightMousePressedLast) {
            glm::ivec3 placePos = block + normal;
            if (world.getVoxelTypeAt(placePos.x, placePos.y, placePos.z) == AIR) {
                world.setVoxel(placePos.x, placePos.y, placePos.z, LOG); // Or any type you want
            }
        }
    }
    leftMousePressedLast = leftMousePressed;
    rightMousePressedLast = rightMousePressed;
}

/**
 * @brief Displays a loading screen while synchronously loading world chunks around the player.
 *
 * Loads all chunks within the render distance surrounding the player's current chunk, updating an ImGui-based progress bar to indicate loading progress. Keeps the window responsive by rendering and polling events during the loading process.
 */
void loadingScreen(GLFWwindow* window, InfiniteWorld& world) {
    ChunkCoord playerChunk = camera.getCurrentChunkCoord();
    int totalChunks = (2 * RENDER_DISTANCE + 1) * (2 * RENDER_DISTANCE + 1);
    int loadedChunks = 0;

    for (int x = playerChunk.x - RENDER_DISTANCE; x <= playerChunk.x + RENDER_DISTANCE; ++x) {
    for (int z = playerChunk.z - RENDER_DISTANCE; z <= playerChunk.z + RENDER_DISTANCE; ++z) {
        world.loadChunk(ChunkCoord(x, z));
        loadedChunks++;

        // Optionally update the loading screen with progress
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH/2 - 100, SCR_HEIGHT/2 - 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_Always);
        ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("Loading world...");
        ImGui::ProgressBar((float)loadedChunks / totalChunks, ImVec2(180, 20));
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
        }
    }
}

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 lightDir = normalize(vec3(1.0, 2.0, 1.0));
uniform vec3 viewPos;

void main() {
    vec3 norm = normalize(Normal);

    // Face shading: brighter top, darker bottom, normal sides
    float faceShade = 1.0;
    if (norm.y > 0.9)      // Top face
        faceShade = 1.1;
    else if (norm.y < -0.9) // Bottom face
        faceShade = 0.7;
    else                   // Sides
        faceShade = 0.9;

    // Ambient
    float ambientStrength = 0.35;
    vec3 ambient = ambientStrength * Color * faceShade;

    // Diffuse
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * Color * faceShade;

    // Specular
    float specularStrength = 0.25;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;

    // Gamma correction
    result = pow(result, vec3(1.0/2.2));

    FragColor = vec4(result, 1.0);
}
)";

/**
 * @brief Entry point for the voxel engine application.
 *
 * Initializes the windowing system, OpenGL context, ImGui UI, and world state. Handles the main application loop, including input processing, camera movement, world updates, rendering, and UI display. Cleans up all resources on exit.
 *
 * @return int Exit status code (0 on success, negative on failure).
 */
int main() {
    // GLFW init
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Voxel Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW or GLAD
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    std::random_device rd;
    GLOBAL_SEED = rd(); // Use a random seed for world generation

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Shader
    GLuint shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);

    // World
    InfiniteWorld world;

    loadingScreen(window, world);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        processInteraction(window, camera, world);

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera/view/projection
        mat4 projection = glm::perspective(glm::radians(70.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 1000.0f);
        mat4 view = camera.getViewMatrix();
        mat4 model = glm::mat4(1.0f);

        // Update world
        world.update(camera);

        // Render world
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        world.render(projection * view);

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        float fps = 1.0f / (deltaTime > 0 ? deltaTime : 1.0f);
        renderUI(camera, fps);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        glDisable(GL_CULL_FACE);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}