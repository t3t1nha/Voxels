#include "Engine/Chunk.h"
#include "Engine/InfiniteWorld.h"
#include "Generation/Noise.h"
#include "Common.h"
#include <GL/glew.h>

Chunk::Chunk(ChunkCoord c, InfiniteWorld* w)
    : coord(c), world(w), meshGenerated(false), meshDirty(true) {
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

    // Define Biomes
    Biome biomes[] = {
        {"Plains", GRASS, DIRT, STONE, 20.0f, 4.0f},
        {"Mountains", SNOW, GRASS, STONE, 32.0f, 18.0f},
        {"Desert", SAND, SAND, STONE, 18.0f, 2.0f},
        {"Forest", GRASS, DIRT, STONE, 22.0f, 5.0f}
    };
    const int biomeCount = sizeof(biomes) / sizeof(Biome);

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            int worldX = coord.x * CHUNK_SIZE + x;
            int worldZ = coord.z * CHUNK_SIZE + z;
            
            // Biome Selection
            float biomeNoise = perlinNoise(worldX * 0.001f, worldZ * 0.001f, GLOBAL_SEED);
            int biomeIndex = int((biomeNoise + 1.0f) * 0.5f * biomeCount) % biomeCount;
            const Biome& biome = biomes[biomeIndex];

            //Height Generation
            float heightNoise = perlinNoise(worldX * 0.01f, worldZ * 0.01f, GLOBAL_SEED);
            int height = int(biome.baseHeight + biome.heightVariation * heightNoise);
            // Clamp height to valid range
            height = std::max(1, std::min(CHUNK_HEIGHT - 1, height));

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y < height - 5) {
                    voxels[x][y][z] = Voxel(biome.filler);
                } else if (y < height - 1) {
                    voxels[x][y][z] = Voxel(biome.subsurface);
                } else if (y < height) {
                    voxels[x][y][z] = Voxel(biome.surface);
                } else if (y < 15 && biome.surface != SAND) {
                    voxels[x][y][z] = Voxel(WATER);
                } else {
                    voxels[x][y][z] = Voxel(AIR);
                }
            }

            // Forest trees (simple, only in forest biome)
            if (biome.name == "Mountains" || biome.name == "Forest" && height < CHUNK_HEIGHT - 6) {
                float treeNoise = perlinNoise(worldX * 0.1f, worldZ * 0.1f, GLOBAL_SEED);
                if (treeNoise > 0.6f) {
                    // Place a simple tree
                    for (int t = 0; t < 4; t++)
                        voxels[x][height + t][z] = Voxel(LOG);
                    for (int dx = -2; dx <= 2; dx++)
                        for (int dz = -2; dz <= 2; dz++)
                            for (int dy = 3; dy <= 5; dy++)
                                if (x + dx >= 0 && x + dx < CHUNK_SIZE &&
                                    z + dz >= 0 && z + dz < CHUNK_SIZE &&
                                    abs(dx) + abs(dz) + (dy - 3) < 5)
                                    voxels[x + dx][height + dy][z + dz] = Voxel(LEAVES);
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
        case STONE:     return vec3(0.5f, 0.5f, 0.5f);
        case GRASS:     return vec3(0.0f, 0.8f, 0.0f);
        case DIRT:      return vec3(0.6f, 0.4f, 0.2f);
        case SAND:      return vec3(0.9f, 0.8f, 0.5f);
        case WATER:     return vec3(0.2f, 0.4f, 0.8f);
        case SNOW:      return vec3(0.95f, 0.98f, 1.0f);
        case LOG:       return vec3(0.55f, 0.27f, 0.07f);
        case LEAVES:    return vec3(0.13f, 0.55f, 0.13f);
        default:        return vec3(1.0f, 1.0f, 1.0f);
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
        std::fill_n(mask, CHUNK_SIZE * CHUNK_HEIGHT, AIR);
        
        // Fill mask - check if face should be rendered
        for (int j = 0; j < dimensions[v]; j++) {
    for (int i = 0; i < dimensions[u]; i++) {
        int pos[3];
        pos[u] = i;
        pos[v] = j;
        pos[w] = d;

        int adjPos[3];
        adjPos[u] = i;
        adjPos[v] = j;
        adjPos[w] = d + direction;

        VoxelType current = getVoxelTypeAt(pos[0], pos[1], pos[2]);
        VoxelType adjacent = getVoxelTypeAt(adjPos[0], adjPos[1], adjPos[2]);

        // Only create face if current is solid and adjacent is NOT solid (air or water)
        bool currentSolid = (current != AIR && current != WATER);
        bool adjacentSolid = (adjacent != AIR && adjacent != WATER);

        if (axis == 1 && direction == -1 && pos[1] == 0) {
        mask[j * dimensions[u] + i] = AIR;
        continue;
        }

        if (currentSolid && !adjacentSolid) {
            mask[j * dimensions[u] + i] = current;
        } else {
            mask[j * dimensions[u] + i] = AIR;
        }
    }
}
        // Process mask to find quads
        // Iterate through mask to find contiguous areas of the same voxel type
        // This will allow us to create larger quads instead of individual faces
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
    // Assign different colors for each face
    vec3 color;
    if (axis == 1 && direction == 1) { // Top face (Y+)
        color = getVoxelColor(voxelType) * vec3(1.0f, 1.0f, 1.0f); // Brighter
    } else if (axis == 1 && direction == -1) { // Bottom face (Y-)
        color = getVoxelColor(voxelType) * vec3(0.7f, 0.7f, 0.7f); // Darker
    } else { // Sides (X/Z)
        color = getVoxelColor(voxelType) * vec3(0.85f, 0.85f, 0.85f); // Slightly dim
    }

    // ...existing code for quad generation...
    // Base position
    int pos[3] = {0, 0, 0};
    pos[u] = i;
    pos[v] = j;
    pos[w] = d;

    // Offsets for quad corners
    int du[3] = {0, 0, 0};
    int dv[3] = {0, 0, 0};
    du[u] = width;
    dv[v] = height;

    // Offset for face direction
    float faceOffset = (direction > 0) ? 1.0f : 0.0f;

    // Four corners (explicitly for each face)
    vec3 corners[4];
    for (int c = 0; c < 4; ++c) {
        int corner[3] = { pos[0], pos[1], pos[2] };
        if (c == 1 || c == 2) corner[u] += du[u];
        if (c == 2 || c == 3) corner[v] += dv[v];
        corner[w] += faceOffset;
        corners[c] = vec3(
            corner[0] + worldPosition.x,
            corner[1] + worldPosition.y,
            corner[2] + worldPosition.z
        );
    }

    // Normal
    vec3 normal(0, 0, 0);
    normal[axis] = direction;

    // Winding order: flip for negative direction
    int quad[4] = {0, 1, 2, 3};
    if (direction < 0)
        std::swap(quad[1], quad[3]);

    // Special case for top face (Y+): ensure correct winding
    if (axis == 1 && direction == 1) {
        std::swap(quad[1], quad[3]);
    }

    // Two triangles
    int triangles[2][3] = { {quad[0], quad[1], quad[2]}, {quad[0], quad[2], quad[3]} };
    for (int t = 0; t < 2; t++) {
        for (int vtx = 0; vtx < 3; vtx++) {
            vec3 vertex = corners[triangles[t][vtx]];
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

        // If out of world bounds (e.g., below 0 or above world height), treat as AIR
        if (worldY < 0 || worldY >= CHUNK_HEIGHT)
            return AIR;

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