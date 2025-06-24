#pragma once
#include "Common.h"
#include "Chunk.h"
#include "Camera.h"
#include "Frustum.h"

class InfiniteWorld {
private:
    std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> chunks;
    ChunkCoord lastPlayerChunk;

    Frustum frustum;    
    
public:
    InfiniteWorld();
    ~InfiniteWorld();
    
    Chunk* getChunk(ChunkCoord coord);
    void update(const Camera& camera);
    void loadChunksAroundPlayer(ChunkCoord playerChunk);
    void unloadDistantChunks(ChunkCoord playerChunk);
    void render(const glm::mat4& viewProkMatrix);
    bool isVoxelSolidAt(int worldX, int worldY, int worldZ);
    void markNeighbourChunksDirty(ChunkCoord coord);
    int getLoadedChunkCount() const;
};