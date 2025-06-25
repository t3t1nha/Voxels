#pragma once
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
#include <string>
#include <algorithm>

using namespace glm;

// Constants
constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_HEIGHT = 64;
constexpr int RENDER_DISTANCE = 4;
const float VOXEL_SIZE = 1.0f;

// Voxel types
enum VoxelType {
    AIR = 0,
    STONE = 1,
    GRASS = 2,
    DIRT = 3,
    SAND = 4,
    WATER = 5,
    SNOW = 6,
    LOG = 7,
    LEAVES = 8
};

struct Biome {
    std::string name;
    VoxelType surface;
    VoxelType subsurface;
    VoxelType filler;
    float baseHeight;
    float heightVariation;
};

// Chunk coordinate structure
struct ChunkCoord {
    int x, z;
    
    ChunkCoord(int x_ = 0, int z_ = 0) : x(x_), z(z_) {}
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }
    bool operator<(const ChunkCoord& other) const {
        return x < other.x || (x == other.x && z < other.z);
    }
};

// Hash function for ChunkCoord
struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const {
        return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.z) << 1);
    }
};

// Forward declarations
class Chunk;
class InfiniteWorld;

// Voxel structure
struct Voxel {
    VoxelType type;
    bool isActive;
    
    Voxel() : type(AIR), isActive(false) {}
    Voxel(VoxelType t) : type(t), isActive(t != AIR) {}
};