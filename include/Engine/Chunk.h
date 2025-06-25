#pragma once
#include "Common.h"

class InfiniteWorld; // Forward declaration

class Chunk {
public:
    ChunkCoord coord;
    InfiniteWorld* world;
    vec3 worldPosition;
    Voxel voxels[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    std::vector<float> vertices;
    bool meshGenerated, meshDirty;
    unsigned int VAO, VBO;

    Chunk(ChunkCoord c, InfiniteWorld* w);
    ~Chunk();
    
    void generateTerrain();
    void generateMesh();
    void generateFacesForDirection(int axis, int direction);
    void addOptimizedQuad(int axis, int direction, int i, int j, int d, int width, int height, int u, int v, int w, VoxelType voxelType);
    void addFace(int x, int y, int z, int face, vec3 color);
    void render();
    bool isVoxelSolidAtPosition(int x, int y, int z);
    VoxelType getVoxelTypeAt(int x, int y, int z);
    vec3 getVoxelColor(VoxelType type);
};