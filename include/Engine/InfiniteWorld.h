#pragma once
#include <map>
#include "Common.h"
#include "Engine/Chunk.h"
#include "Engine/Camera.h"
#include "Frustum.h"

/**
 * Loads the chunk at the specified chunk coordinates into memory.
 * @param coord The coordinates of the chunk to load.
 */
class InfiniteWorld {
public:
    std::map<ChunkCoord, Chunk*> chunks;
    ChunkCoord lastPlayerChunk;
    Frustum frustum;

    InfiniteWorld();
    ~InfiniteWorld();

    Chunk* getChunk(ChunkCoord coord);
    void update(const Camera& camera);
    void loadChunk(ChunkCoord coord);
    void loadChunksAroundPlayer(ChunkCoord playerChunk);
    void unloadDistantChunks(ChunkCoord playerChunk);
    void render(const glm::mat4& viewProj);
    bool isVoxelSolidAt(int worldX, int worldY, int worldZ);
    void setVoxel(int worldX, int worldY, int worldZ, VoxelType type);
    void markNeighbourChunksDirty(ChunkCoord coord);
    int getLoadedChunkCount() const;
    VoxelType getVoxelTypeAt(int worldX, int worldY, int worldZ);
};