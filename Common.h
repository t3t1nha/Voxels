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

using namespace glm;

// Constants
const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 64;
const int RENDER_DISTANCE = 20;
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