#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace glm;

// Constants
const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 64;
const int RENDER_DISTANCE = 8; // How many chunks to render around player
const float VOXEL_SIZE = 1.0f;

// Voxel types
enum VoxelType {
    AIR = 0,
    STONE = 1,
    GRASS = 2,
    DIRT = 3,
    SAND = 4,
    WATER = 5
};

// Chunk coordinate structure
struct ChunkCoord {
    int x, z;
    
    ChunkCoord(int x = 0, int z = 0) : x(x), z(z) {}
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }
};

// Hash function for ChunkCoord to use in unordered_map
struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const {
        // Simple hash combining x and z coordinates
        return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.z) << 1);
    }
};

// Camera class (same as before)
class Camera {
public:
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;
    
    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    
    Camera(vec3 pos = vec3(0.0f, 32.0f, 0.0f)) {
        position = pos;
        worldUp = vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        movementSpeed = 15.0f;
        mouseSensitivity = 0.1f;
        updateCameraVectors();
    }
    
    mat4 getViewMatrix() {
        return lookAt(position, position + front, up);
    }
    
    void processKeyboard(int direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        if (direction == 0) position += front * velocity; // W
        if (direction == 1) position -= front * velocity; // S
        if (direction == 2) position -= right * velocity; // A
        if (direction == 3) position += right * velocity; // D
        if (direction == 4) position += up * velocity;    // Space
        if (direction == 5) position -= up * velocity;    // Shift
    }
    
    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        updateCameraVectors();
    }
    
    // Get the chunk coordinate the camera is currently in
    ChunkCoord getCurrentChunkCoord() const {
        int chunkX = (int)floor(position.x / CHUNK_SIZE);
        int chunkZ = (int)floor(position.z / CHUNK_SIZE);
        return ChunkCoord(chunkX, chunkZ);
    }
    
private:
    void updateCameraVectors() {
        vec3 front_;
        front_.x = cos(radians(yaw)) * cos(radians(pitch));
        front_.y = sin(radians(pitch));
        front_.z = sin(radians(yaw)) * cos(radians(pitch));
        front = normalize(front_);
        right = normalize(cross(front, worldUp));
        up = normalize(cross(right, front));
    }
};

// Voxel structure
struct Voxel {
    VoxelType type;
    bool isActive;
    
    Voxel() : type(AIR), isActive(false) {}
    Voxel(VoxelType t) : type(t), isActive(t != AIR) {}
};

// Simple noise function for terrain generation
float noise(int x, int z, int seed = 12345) {
    int n = x + z * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float smoothNoise(float x, float z) {
    float corners = (noise(x-1, z-1) + noise(x+1, z-1) + noise(x-1, z+1) + noise(x+1, z+1)) / 16.0f;
    float sides = (noise(x-1, z) + noise(x+1, z) + noise(x, z-1) + noise(x, z+1)) / 8.0f;
    float center = noise(x, z) / 4.0f;
    return corners + sides + center;
}

float interpolatedNoise(float x, float z) {
    int intX = (int)x;
    float fracX = x - intX;
    int intZ = (int)z;
    float fracZ = z - intZ;
    
    float v1 = smoothNoise(intX, intZ);
    float v2 = smoothNoise(intX + 1, intZ);
    float v3 = smoothNoise(intX, intZ + 1);
    float v4 = smoothNoise(intX + 1, intZ + 1);
    
    float i1 = v1 * (1 - fracX) + v2 * fracX;
    float i2 = v3 * (1 - fracX) + v4 * fracX;
    
    return i1 * (1 - fracZ) + i2 * fracZ;
}

float perlinNoise(float x, float z) {
    float total = 0;
    float persistence = 0.5f;
    int octaves = 4;
    
    for (int i = 0; i < octaves; i++) {
        float frequency = pow(2, i);
        float amplitude = pow(persistence, i);
        total += interpolatedNoise(x * frequency, z * frequency) * amplitude;
    }
    
    return total;
}

// Chunk class
class Chunk {
public:
    Voxel voxels[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    ChunkCoord coord;
    vec3 worldPosition;
    GLuint VAO, VBO;
    std::vector<float> vertices;
    bool meshGenerated;
    bool meshDirty;
    
    Chunk(ChunkCoord c) : coord(c), meshGenerated(false), meshDirty(true) {
        worldPosition = vec3(coord.x * CHUNK_SIZE, 0, coord.z * CHUNK_SIZE);
        generateTerrain();
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }
    
    ~Chunk() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    
    void generateTerrain() {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                // World coordinates
                int worldX = coord.x * CHUNK_SIZE + x;
                int worldZ = coord.z * CHUNK_SIZE + z;
                
                // Generate height using Perlin noise
                float noiseValue = perlinNoise(worldX * 0.01f, worldZ * 0.01f);
                int height = 20 + (int)(15 * noiseValue);
                
                // Clamp height to chunk bounds
                height = std::max(0, std::min(CHUNK_HEIGHT - 1, height));
                
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    if (y < height - 8) {
                        voxels[x][y][z] = Voxel(STONE);
                    } else if (y < height - 2) {
                        voxels[x][y][z] = Voxel(DIRT);
                    } else if (y < height) {
                        // Add some variation - sometimes sand near water level
                        if (height < 18) {
                            voxels[x][y][z] = Voxel(SAND);
                        } else {
                            voxels[x][y][z] = Voxel(GRASS);
                        }
                    } else if (y < 15) {
                        voxels[x][y][z] = Voxel(WATER);
                    } else {
                        voxels[x][y][z] = Voxel(AIR);
                    }
                }
            }
        }
    }
    
    bool isVoxelSolid(int x, int y, int z) {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return false;
        return voxels[x][y][z].isActive;
    }
    
    vec3 getVoxelColor(VoxelType type) {
        switch (type) {
            case STONE: return vec3(0.5f, 0.5f, 0.5f);
            case GRASS: return vec3(0.0f, 0.8f, 0.0f);
            case DIRT:  return vec3(0.6f, 0.4f, 0.2f);
            case SAND:  return vec3(0.9f, 0.8f, 0.5f);
            case WATER: return vec3(0.2f, 0.4f, 0.8f);
            default:    return vec3(1.0f, 1.0f, 1.0f);
        }
    }
    
    void addFace(int x, int y, int z, int face, vec3 color) {
        float vx = x + worldPosition.x;
        float vy = y + worldPosition.y;
        float vz = z + worldPosition.z;
        
        // Face vertices (position + normal + color)
        float faceVertices[6][4][9] = {
            // Front face (Z+)
            {
                {vx,     vy,     vz+1, 0,0,1, color.r, color.g, color.b},
                {vx+1,   vy,     vz+1, 0,0,1, color.r, color.g, color.b},
                {vx+1,   vy+1,   vz+1, 0,0,1, color.r, color.g, color.b},
                {vx,     vy+1,   vz+1, 0,0,1, color.r, color.g, color.b}
            },
            // Back face (Z-)
            {
                {vx+1,   vy,     vz, 0,0,-1, color.r, color.g, color.b},
                {vx,     vy,     vz, 0,0,-1, color.r, color.g, color.b},
                {vx,     vy+1,   vz, 0,0,-1, color.r, color.g, color.b},
                {vx+1,   vy+1,   vz, 0,0,-1, color.r, color.g, color.b}
            },
            // Left face (X-)
            {
                {vx,     vy,     vz, -1,0,0, color.r, color.g, color.b},
                {vx,     vy,     vz+1, -1,0,0, color.r, color.g, color.b},
                {vx,     vy+1,   vz+1, -1,0,0, color.r, color.g, color.b},
                {vx,     vy+1,   vz, -1,0,0, color.r, color.g, color.b}
            },
            // Right face (X+)
            {
                {vx+1,   vy,     vz+1, 1,0,0, color.r, color.g, color.b},
                {vx+1,   vy,     vz, 1,0,0, color.r, color.g, color.b},
                {vx+1,   vy+1,   vz, 1,0,0, color.r, color.g, color.b},
                {vx+1,   vy+1,   vz+1, 1,0,0, color.r, color.g, color.b}
            },
            // Bottom face (Y-)
            {
                {vx,     vy,     vz, 0,-1,0, color.r, color.g, color.b},
                {vx+1,   vy,     vz, 0,-1,0, color.r, color.g, color.b},
                {vx+1,   vy,     vz+1, 0,-1,0, color.r, color.g, color.b},
                {vx,     vy,     vz+1, 0,-1,0, color.r, color.g, color.b}
            },
            // Top face (Y+)
            {
                {vx,     vy+1,   vz+1, 0,1,0, color.r, color.g, color.b},
                {vx+1,   vy+1,   vz+1, 0,1,0, color.r, color.g, color.b},
                {vx+1,   vy+1,   vz, 0,1,0, color.r, color.g, color.b},
                {vx,     vy+1,   vz, 0,1,0, color.r, color.g, color.b}
            }
        };
        
        // Add two triangles for the quad
        int indices[] = {0, 1, 2, 0, 2, 3};
        for (int i = 0; i < 6; i++) {
            int vertIndex = indices[i];
            for (int j = 0; j < 9; j++) {
                vertices.push_back(faceVertices[face][vertIndex][j]);
            }
        }
    }
    
    void generateMesh() {
        vertices.clear();
        
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    if (!voxels[x][y][z].isActive) continue;
                    
                    vec3 color = getVoxelColor(voxels[x][y][z].type);
                    
                    // Check each face and add if not occluded
                    if (!isVoxelSolid(x, y, z+1)) addFace(x, y, z, 0, color); // Front
                    if (!isVoxelSolid(x, y, z-1)) addFace(x, y, z, 1, color); // Back
                    if (!isVoxelSolid(x-1, y, z)) addFace(x, y, z, 2, color); // Left
                    if (!isVoxelSolid(x+1, y, z)) addFace(x, y, z, 3, color); // Right
                    if (!isVoxelSolid(x, y-1, z)) addFace(x, y, z, 4, color); // Bottom
                    if (!isVoxelSolid(x, y+1, z)) addFace(x, y, z, 5, color); // Top
                }
            }
        }
        
        // Upload to GPU
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.empty() ? nullptr : &vertices[0], GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Color attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        meshGenerated = true;
        meshDirty = false;
    }
    
    void render() {
        if (!meshGenerated || meshDirty) {
            generateMesh();
        }
        
        if (!vertices.empty()) {
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 9);
            glBindVertexArray(0);
        }
    }
};

// Infinite World class
class InfiniteWorld {
private:
    std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> chunks;
    ChunkCoord lastPlayerChunk;
    
public:
    InfiniteWorld() {
        lastPlayerChunk = ChunkCoord(0, 0);
    }
    
    ~InfiniteWorld() {
        // Clean up all chunks
        for (auto& pair : chunks) {
            delete pair.second;
        }
        chunks.clear();
    }
    
    // Get chunk at specific coordinate, create if doesn't exist
    Chunk* getChunk(ChunkCoord coord) {
        auto it = chunks.find(coord);
        if (it != chunks.end()) {
            return it->second;
        }
        
        // Create new chunk
        Chunk* newChunk = new Chunk(coord);
        chunks[coord] = newChunk;
        std::cout << "Generated chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;
        return newChunk;
    }
    
    // Update world based on player position
    void update(const Camera& camera) {
        ChunkCoord playerChunk = camera.getCurrentChunkCoord();
        
        // Only update if player moved to a different chunk
        if (!(playerChunk == lastPlayerChunk)) {
            std::cout << "Player moved to chunk (" << playerChunk.x << ", " << playerChunk.z << ")" << std::endl;
            lastPlayerChunk = playerChunk;
            
            // Generate chunks around player
            loadChunksAroundPlayer(playerChunk);
            
            // Unload distant chunks
            unloadDistantChunks(playerChunk);
        }
    }
    
    void loadChunksAroundPlayer(ChunkCoord playerChunk) {
        // Load chunks in a square around the player
        for (int x = playerChunk.x - RENDER_DISTANCE; x <= playerChunk.x + RENDER_DISTANCE; x++) {
            for (int z = playerChunk.z - RENDER_DISTANCE; z <= playerChunk.z + RENDER_DISTANCE; z++) {
                ChunkCoord coord(x, z);
                
                // Only create chunk if it doesn't exist
                if (chunks.find(coord) == chunks.end()) {
                    getChunk(coord);
                }
            }
        }
    }
    
    void unloadDistantChunks(ChunkCoord playerChunk) {
        std::vector<ChunkCoord> chunksToRemove;
        
        // Find chunks that are too far away
        for (auto& pair : chunks) {
            ChunkCoord chunkCoord = pair.first;
            int dx = abs(chunkCoord.x - playerChunk.x);
            int dz = abs(chunkCoord.z - playerChunk.z);
            
            // If chunk is outside render distance + buffer, mark for removal
            if (dx > RENDER_DISTANCE + 2 || dz > RENDER_DISTANCE + 2) {
                chunksToRemove.push_back(chunkCoord);
            }
        }
        
        // Remove distant chunks
        for (ChunkCoord coord : chunksToRemove) {
            auto it = chunks.find(coord);
            if (it != chunks.end()) {
                std::cout << "Unloaded chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;
                delete it->second;
                chunks.erase(it);
            }
        }
    }
    
    void render() {
        // Render all loaded chunks
        for (auto& pair : chunks) {
            pair.second->render();
        }
    }
    
    // Get statistics for debugging
    int getLoadedChunkCount() const {
        return chunks.size();
    }
};

// Shader source code (same as before)
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;
out vec3 LightPos;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    LightPos = lightPos;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec3 LightPos;

out vec4 FragColor;

void main() {
    vec3 lightColor = vec3(1.0, 1.0, 0.9);
    vec3 ambient = 0.4 * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 result = (ambient + diffuse) * Color;
    FragColor = vec4(result, 1.0);
}
)";

// Global variables
Camera camera;
InfiniteWorld* world;
bool firstMouse = true;
float lastX = 600, lastY = 400;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Input handling
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
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
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    
    camera.processMouseMovement(xoffset, yoffset);
}

GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    return shader;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Infinite Voxel World", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    // Enable OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Compile shaders
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create infinite world
    world = new InfiniteWorld();
    
    std::cout << "=== INFINITE VOXEL ENGINE ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "WASD - Move horizontally" << std::endl;
    std::cout << "Space - Move up" << std::endl;
    std::cout << "Shift - Move down" << std::endl;
    std::cout << "Mouse - Look around" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << std::endl;
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        
        // Update world (generate/unload chunks based on player position)
        world->update(camera);
        
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Set matrices
        mat4 model = mat4(1.0f);
        mat4 view = camera.getViewMatrix();
        mat4 projection = perspective(radians(45.0f), 1200.0f / 800.0f, 0.1f, 1000.0f);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
        
        vec3 lightPos = camera.position + vec3(100.0f, 100.0f, 100.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, value_ptr(lightPos));
        
        // Render the infinite world
        world->render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    delete world;
    glfwTerminate();
    return 0;
}