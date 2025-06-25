#include "Common.h"
#include "Camera.h"
#include "Chunk.h"
#include "InfiniteWorld.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Initialize ImGui (after GLFW/GLEW init)
void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();
}

// Render UI overlay
void renderUI(const Camera& camera, float fps) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a transparent overlay window
    ImGui::SetNextWindowBgAlpha(0.3f);
    ImGui::Begin("Debug Info", nullptr, 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize);

    // Display camera position
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", 
        camera.position.x, 
        camera.position.y, 
        camera.position.z);

    // Display FPS
    ImGui::Text("FPS: %.1f", fps);

    // Chunk coordinates
    ChunkCoord coord = camera.getCurrentChunkCoord();
    ImGui::Text("Chunk: [%d, %d]", coord.x, coord.z);

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Noise functions for terrain generation
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

// Camera implementation
Camera::Camera(vec3 pos) {
    position = pos;
    worldUp = vec3(0.0f, 1.0f, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    movementSpeed = 15.0f;
    mouseSensitivity = 0.1f;
    updateCameraVectors();
}

mat4 Camera::getViewMatrix() {
    return lookAt(position, position + front, up);
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (direction == 0) position += front * velocity; // W
    if (direction == 1) position -= front * velocity; // S
    if (direction == 2) position -= right * velocity; // A
    if (direction == 3) position += right * velocity; // D
    if (direction == 4) position += up * velocity;    // Space
    if (direction == 5) position -= up * velocity;    // Shift
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;
    
    yaw += xoffset;
    pitch += yoffset;
    
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    
    updateCameraVectors();
}

ChunkCoord Camera::getCurrentChunkCoord() const {
    int chunkX = (int)floor(position.x / CHUNK_SIZE);
    int chunkZ = (int)floor(position.z / CHUNK_SIZE);
    return ChunkCoord(chunkX, chunkZ);
}

void Camera::updateCameraVectors() {
    vec3 front_;
    front_.x = cos(radians(yaw)) * cos(radians(pitch));
    front_.y = sin(radians(pitch));
    front_.z = sin(radians(yaw)) * cos(radians(pitch));
    front = normalize(front_);
    right = normalize(cross(front, worldUp));
    up = normalize(cross(right, front));
}

// Chunk implementation
Chunk::Chunk(ChunkCoord c, InfiniteWorld* w) : coord(c), world(w), meshGenerated(false), meshDirty(true) {
    worldPosition = vec3(coord.x * CHUNK_SIZE, 0, coord.z * CHUNK_SIZE);
    generateTerrain();
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Chunk::~Chunk() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Chunk::generateTerrain() {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = coord.x * CHUNK_SIZE + x;
            int worldZ = coord.z * CHUNK_SIZE + z;
            
            float noiseValue = perlinNoise(worldX * 0.01f, worldZ * 0.01f);
            int height = 20 + (int)(15 * noiseValue);
            height = std::max(0, std::min(CHUNK_HEIGHT - 1, height));
            
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y < height - 8) {
                    voxels[x][y][z] = Voxel(STONE);
                } else if (y < height - 2) {
                    voxels[x][y][z] = Voxel(DIRT);
                } else if (y < height) {
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

bool Chunk::isVoxelSolidAtPosition(int x, int y, int z) {
    int worldX = coord.x * CHUNK_SIZE + x;
    int worldY = y;
    int worldZ = coord.z * CHUNK_SIZE + z;
    return world->isVoxelSolidAt(worldX, worldY, worldZ);
}

vec3 Chunk::getVoxelColor(VoxelType type) {
    switch (type) {
        case STONE: return vec3(0.5f, 0.5f, 0.5f);
        case GRASS: return vec3(0.0f, 0.8f, 0.0f);
        case DIRT:  return vec3(0.6f, 0.4f, 0.2f);
        case SAND:  return vec3(0.9f, 0.8f, 0.5f);
        case WATER: return vec3(0.2f, 0.4f, 0.8f);
        default:    return vec3(1.0f, 1.0f, 1.0f);
    }
}

void Chunk::addFace(int x, int y, int z, int face, vec3 color) {
    float vx = x + worldPosition.x;
    float vy = y + worldPosition.y;
    float vz = z + worldPosition.z;
    
    float faceVertices[6][4][9] = {
        // Front face (Z+)
        {{vx,vy,vz+1,0,0,1,color.r,color.g,color.b},{vx+1,vy,vz+1,0,0,1,color.r,color.g,color.b},
         {vx+1,vy+1,vz+1,0,0,1,color.r,color.g,color.b},{vx,vy+1,vz+1,0,0,1,color.r,color.g,color.b}},
        // Back face (Z-)
        {{vx+1,vy,vz,0,0,-1,color.r,color.g,color.b},{vx,vy,vz,0,0,-1,color.r,color.g,color.b},
         {vx,vy+1,vz,0,0,-1,color.r,color.g,color.b},{vx+1,vy+1,vz,0,0,-1,color.r,color.g,color.b}},
        // Left face (X-)
        {{vx,vy,vz,-1,0,0,color.r,color.g,color.b},{vx,vy,vz+1,-1,0,0,color.r,color.g,color.b},
         {vx,vy+1,vz+1,-1,0,0,color.r,color.g,color.b},{vx,vy+1,vz,-1,0,0,color.r,color.g,color.b}},
        // Right face (X+)
        {{vx+1,vy,vz+1,1,0,0,color.r,color.g,color.b},{vx+1,vy,vz,1,0,0,color.r,color.g,color.b},
         {vx+1,vy+1,vz,1,0,0,color.r,color.g,color.b},{vx+1,vy+1,vz+1,1,0,0,color.r,color.g,color.b}},
        // Bottom face (Y-)
        {{vx,vy,vz,0,-1,0,color.r,color.g,color.b},{vx+1,vy,vz,0,-1,0,color.r,color.g,color.b},
         {vx+1,vy,vz+1,0,-1,0,color.r,color.g,color.b},{vx,vy,vz+1,0,-1,0,color.r,color.g,color.b}},
        // Top face (Y+)
        {{vx,vy+1,vz+1,0,1,0,color.r,color.g,color.b},{vx+1,vy+1,vz+1,0,1,0,color.r,color.g,color.b},
         {vx+1,vy+1,vz,0,1,0,color.r,color.g,color.b},{vx,vy+1,vz,0,1,0,color.r,color.g,color.b}}
    };
    
    int indices[] = {0,1,2,0,2,3};
    for (int i = 0; i < 6; i++) {
        int vertIndex = indices[i];
        for (int j = 0; j < 9; j++) {
            vertices.push_back(faceVertices[face][vertIndex][j]);
        }
    }
}

// Fixed Greedy Meshing Implementation - Replace your generateMesh() method

void Chunk::generateMesh() {
    vertices.clear();
    
    // Generate mesh for each of the 6 face directions
    generateFacesForDirection(0, 1);   // +X faces
    generateFacesForDirection(0, -1);  // -X faces
    generateFacesForDirection(1, 1);   // +Y faces
    generateFacesForDirection(1, -1);  // -Y faces
    generateFacesForDirection(2, 1);   // +Z faces
    generateFacesForDirection(2, -1);  // -Z faces
    
    // Upload vertices to GPU
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), 
                 vertices.empty() ? nullptr : &vertices[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    meshGenerated = true;
    meshDirty = false;
}

void Chunk::generateFacesForDirection(int axis, int direction) {
    // Define dimensions based on axis
    int dimensions[3] = {CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE};
    
    // Create coordinate mapping
    int u, v, w;
    if (axis == 0) { // X axis
        u = 1; v = 2; w = 0; // u=Y, v=Z, w=X
    } else if (axis == 1) { // Y axis
        u = 0; v = 2; w = 1; // u=X, v=Z, w=Y
    } else { // Z axis
        u = 0; v = 1; w = 2; // u=X, v=Y, w=Z
    }
    
    // Iterate through each slice perpendicular to the axis
    for (int d = 0; d < dimensions[axis]; d++) {
        // Create mask for this slice
        VoxelType mask[CHUNK_SIZE * CHUNK_HEIGHT];
        memset(mask, 0, sizeof(mask));
        
        // Fill mask - check if face should be rendered
        for (int j = 0; j < dimensions[v]; j++) {
            for (int i = 0; i < dimensions[u]; i++) {
                // Build current position
                int pos[3];
                pos[u] = i;
                pos[v] = j;
                pos[w] = d;
                
                // Build adjacent position
                int adjPos[3];
                adjPos[u] = i;
                adjPos[v] = j;
                adjPos[w] = d + direction;
                
                VoxelType current = getVoxelTypeAt(pos[0], pos[1], pos[2]);
                VoxelType adjacent = getVoxelTypeAt(adjPos[0], adjPos[1], adjPos[2]);
                
                // Create face if current is solid and adjacent is air
                if (current != AIR && adjacent == AIR) {
                    mask[j * dimensions[u] + i] = current;
                }
            }
        }
        
        // Generate quads from mask using greedy meshing
        for (int j = 0; j < dimensions[v]; j++) {
            for (int i = 0; i < dimensions[u]; ) {
                if (mask[j * dimensions[u] + i] != AIR) {
                    VoxelType voxelType = mask[j * dimensions[u] + i];
                    
                    // Find width of quad
                    int width = 1;
                    for (int k = i + 1; k < dimensions[u]; k++) {
                        if (mask[j * dimensions[u] + k] == voxelType) {
                            width++;
                        } else {
                            break;
                        }
                    }
                    
                    // Find height of quad
                    int height = 1;
                    bool canExtend = true;
                    for (int k = j + 1; k < dimensions[v] && canExtend; k++) {
                        for (int l = i; l < i + width; l++) {
                            if (mask[k * dimensions[u] + l] != voxelType) {
                                canExtend = false;
                                break;
                            }
                        }
                        if (canExtend) {
                            height++;
                        }
                    }
                    
                    // Create the quad
                    addOptimizedQuad(axis, direction, i, j, d, width, height, u, v, w, voxelType);
                    
                    // Clear processed area from mask
                    for (int dv = 0; dv < height; dv++) {
                        for (int du = 0; du < width; du++) {
                            mask[(j + dv) * dimensions[u] + (i + du)] = AIR;
                        }
                    }
                    
                    i += width;
                } else {
                    i++;
                }
            }
        }
    }
}

void Chunk::addOptimizedQuad(int axis, int direction, int i, int j, int d, 
                            int width, int height, int u, int v, int w, VoxelType voxelType) {
    vec3 color = getVoxelColor(voxelType);
    
    // Calculate the four corners of the quad
    vec3 corners[4];
    vec3 normal(0, 0, 0);
    
    // Set normal based on axis and direction
    normal[axis] = direction;
    
    // Calculate world-space position offset
    float offset = (direction > 0) ? 1.0f : 0.0f;
    
    // Build quad vertices based on axis
    if (axis == 0) { // X faces
        float x = d + worldPosition.x + offset;
        corners[0] = vec3(x, j + worldPosition.y, i + worldPosition.z);
        corners[1] = vec3(x, j + worldPosition.y, i + width + worldPosition.z);
        corners[2] = vec3(x, j + height + worldPosition.y, i + width + worldPosition.z);
        corners[3] = vec3(x, j + height + worldPosition.y, i + worldPosition.z);
        
        if (direction < 0) {
            std::swap(corners[1], corners[3]);
        }
    } else if (axis == 1) { // Y faces
        float y = d + worldPosition.y + offset;
        corners[0] = vec3(i + worldPosition.x, y, j + worldPosition.z);
        corners[1] = vec3(i + width + worldPosition.x, y, j + worldPosition.z);
        corners[2] = vec3(i + width + worldPosition.x, y, j + height + worldPosition.z);
        corners[3] = vec3(i + worldPosition.x, y, j + height + worldPosition.z);
        
        if (direction > 0) {
            std::swap(corners[1], corners[3]);
        }
    } else { // Z faces
        float z = d + worldPosition.z + offset;
        corners[0] = vec3(i + worldPosition.x, j + worldPosition.y, z);
        corners[1] = vec3(i + width + worldPosition.x, j + worldPosition.y, z);
        corners[2] = vec3(i + width + worldPosition.x, j + height + worldPosition.y, z);
        corners[3] = vec3(i + worldPosition.x, j + height + worldPosition.y, z);
        
        if (direction < 0) {
            std::swap(corners[1], corners[3]);
        }
    }
    
    // Add triangles to vertex buffer
    int triangles[2][3] = {{0, 1, 2}, {0, 2, 3}};
    
    for (int t = 0; t < 2; t++) {
        for (int v = 0; v < 3; v++) {
            vec3 vertex = corners[triangles[t][v]];
            
            // Position
            vertices.push_back(vertex.x);
            vertices.push_back(vertex.y);
            vertices.push_back(vertex.z);
            
            // Normal
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            
            // Color
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
    }
}

VoxelType Chunk::getVoxelTypeAt(int x, int y, int z) {
    // Handle bounds checking
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
        // Query adjacent chunks through world
        int worldX = coord.x * CHUNK_SIZE + x;
        int worldY = y;
        int worldZ = coord.z * CHUNK_SIZE + z;
        
        return world->getVoxelTypeAt(worldX, worldY, worldZ);
    }
    
    return voxels[x][y][z].isActive ? voxels[x][y][z].type : AIR;
}


void Chunk::render() {
    if (!meshGenerated || meshDirty) {
        generateMesh();
    }
    
    if (!vertices.empty()) {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 9);
        glBindVertexArray(0);
    }
}

// InfiniteWorld implementation
InfiniteWorld::InfiniteWorld() {
    lastPlayerChunk = ChunkCoord(0, 0);
}

InfiniteWorld::~InfiniteWorld() {
    for (auto& pair : chunks) {
        delete pair.second;
    }
    chunks.clear();
}

Chunk* InfiniteWorld::getChunk(ChunkCoord coord) {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        return it->second;
    }
    
    Chunk* newChunk = new Chunk(coord, this);
    chunks[coord] = newChunk;
    markNeighbourChunksDirty(coord);
    std::cout << "Generated chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;
    return newChunk;
}

void InfiniteWorld::update(const Camera& camera) {
    ChunkCoord playerChunk = camera.getCurrentChunkCoord();
    
    if (!(playerChunk == lastPlayerChunk)) {
        std::cout << "Player moved to chunk (" << playerChunk.x << ", " << playerChunk.z << ")" << std::endl;
        lastPlayerChunk = playerChunk;
        loadChunksAroundPlayer(playerChunk);
        unloadDistantChunks(playerChunk);
    }
}

void InfiniteWorld::loadChunksAroundPlayer(ChunkCoord playerChunk) {
    for (int x = playerChunk.x - RENDER_DISTANCE; x <= playerChunk.x + RENDER_DISTANCE; x++) {
        for (int z = playerChunk.z - RENDER_DISTANCE; z <= playerChunk.z + RENDER_DISTANCE; z++) {
            ChunkCoord coord(x, z);
            if (chunks.find(coord) == chunks.end()) {
                getChunk(coord);
            }
        }
    }
}

void InfiniteWorld::unloadDistantChunks(ChunkCoord playerChunk) {
    std::vector<ChunkCoord> chunksToRemove;
    
    for (auto& pair : chunks) {
        ChunkCoord chunkCoord = pair.first;
        int dx = abs(chunkCoord.x - playerChunk.x);
        int dz = abs(chunkCoord.z - playerChunk.z);
        
        if (dx > RENDER_DISTANCE + 2 || dz > RENDER_DISTANCE + 2) {
            chunksToRemove.push_back(chunkCoord);
        }
    }
    
    for (ChunkCoord coord : chunksToRemove) {
        auto it = chunks.find(coord);
        if (it != chunks.end()) {
            std::cout << "Unloaded chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;
            delete it->second;
            chunks.erase(it);
        }
    }
}

void InfiniteWorld::render(const glm::mat4& viewProj) {
    frustum.update(viewProj);
    
    for (auto& [coord, chunk] : chunks) {
        glm::vec3 min(
            coord.x * CHUNK_SIZE,
            0,
            coord.z * CHUNK_SIZE
        );
        
        glm::vec3 max(
            min.x + CHUNK_SIZE,
            CHUNK_HEIGHT,
            min.z + CHUNK_SIZE
        );

        if (frustum.isBoxVisible(min, max)) {
            chunk->render();
        }
    }
}

bool InfiniteWorld::isVoxelSolidAt(int worldX, int worldY, int worldZ) {
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    ChunkCoord coord(chunkX, chunkZ);
    auto it = chunks.find(coord);
    if (it == chunks.end()) {
        return false;
    }

    Chunk* chunk = it->second;

    int localX = worldX - (chunkX * CHUNK_SIZE);
    int localY = worldY;
    int localZ = worldZ - (chunkZ * CHUNK_SIZE);

    if (localX < 0 || localX >= CHUNK_SIZE ||
        localY < 0 || localY >= CHUNK_HEIGHT || 
        localZ < 0 || localZ >= CHUNK_SIZE) {
        return false;
    }

    return chunk->voxels[localX][localY][localZ].isActive;
}

void InfiniteWorld::markNeighbourChunksDirty(ChunkCoord coord) {
    ChunkCoord neighbours[4] = {
        ChunkCoord(coord.x - 1, coord.z),
        ChunkCoord(coord.x + 1, coord.z),
        ChunkCoord(coord.x, coord.z - 1),
        ChunkCoord(coord.x, coord.z + 1)
    };

    for (int i = 0; i < 4; i++) {
        auto it = chunks.find(neighbours[i]);
        if (it != chunks.end()) {
            it->second->meshDirty = true;
        }
    }
}

int InfiniteWorld::getLoadedChunkCount() const {
    return chunks.size();
}

VoxelType InfiniteWorld::getVoxelTypeAt(int worldX, int worldY, int worldZ) {
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    ChunkCoord coord(chunkX, chunkZ);
    auto it = chunks.find(coord);
    if (it == chunks.end()) {
        return AIR;
    }

    Chunk* chunk = it->second;

    int localX = worldX - (chunkX * CHUNK_SIZE);
    int localY = worldY;
    int localZ = worldZ - (chunkZ * CHUNK_SIZE);

    if (localX < 0 || localX >= CHUNK_SIZE ||
        localY < 0 || localY >= CHUNK_HEIGHT || 
        localZ < 0 || localZ >= CHUNK_SIZE) {
        return AIR;
    }

    return chunk->voxels[localX][localY][localZ].isActive ? 
           chunk->voxels[localX][localY][localZ].type : AIR;
}


// Shader code
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
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
        static bool showUI = true;
        showUI = !showUI;
        ImGui::GetIO().WantCaptureKeyboard = showUI;
    }
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
    
    initImGui(window);

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
    std::cout << "F1 - Toogle Debug Menu" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << std::endl;
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Calculate FPS
        static float lastTime = 0.0f;
        float currentTime = glfwGetTime();
        float fps = 1.0f / (currentTime - lastTime);
        lastTime = currentTime;

        processInput(window);
        
        // Update world
        world->update(camera);
        
        glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Set matrices
        mat4 model = mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = perspective(radians(45.0f), 1200.0f / 800.0f, 0.1f, 1000.0f);
        glm::mat4 viewProj = projection * view;
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
        
        vec3 lightPos = camera.position + vec3(100.0f, 100.0f, 100.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, value_ptr(lightPos));
        
        // Debug print
        std::cout << "ViewProj Matrix:\n";
        for (int i = 0; i < 4; i++) {
        std::cout << viewProj[i].x << " " << viewProj[i].y << " " << viewProj[i].z << " " << viewProj[i].w << "\n";
}

        // Render the infinite world
        world->render(viewProj);

        // Then render UI on top
        renderUI(camera, fps);
        

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete world;
    glfwTerminate();
    return 0;
}