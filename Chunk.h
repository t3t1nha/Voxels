#pragma once
#include "Common.h"

class InfiniteWorld; // Forward declaration

class Chunk {
public:
    Voxel voxels[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    ChunkCoord coord;
    vec3 worldPosition;
    GLuint VAO, VBO;
    std::vector<float> vertices;
    bool meshGenerated;
    bool meshDirty;

    InfiniteWorld* world;
    
    Chunk(ChunkCoord c, InfiniteWorld* w);
    ~Chunk();
    
    void generateTerrain();
    bool isVoxelSolidAtPosition(int x, int y, int z);
    vec3 getVoxelColor(VoxelType type);
    void addFace(int x, int y, int z, int face, vec3 color);
    void generateMesh();
    void render();
    void generateFacesForDirection(int axis, int direction);
    void addOptimizedQuad(int axis, int direction, int i, int j, int d, 
                         int width, int height, int u, int v, int w, VoxelType voxelType);
    VoxelType getVoxelTypeAt(int x, int y, int z);
};