#pragma once
#include "Common.h"
#include "Chunk.h"
#include "Camera.h"

class InfiniteWorld {
private:
    std::unordered_map<ChunkCoord, Chunk*, ChunkCoordHash> chunks;
    ChunkCoord lastPlayerChunk;
    
public:
    InfiniteWorld();
    ~InfiniteWorld();
    
    Chunk* getChunk(ChunkCoord coord);
    void update(const Camera& camera);
    void loadChunksAroundPlayer(ChunkCoord playerChunk);
    void unloadDistantChunks(ChunkCoord playerChunk);
    void render();
    bool isVoxelSolidAt(int worldX, int worldY, int worldZ);
    void markNeighbourChunksDirty(ChunkCoord coord);
    int getLoadedChunkCount() const;
};