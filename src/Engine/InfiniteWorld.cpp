#include "Engine/InfiniteWorld.h"
#include <iostream>

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
    return newChunk;
}

void InfiniteWorld::update(const Camera& camera) {
    ChunkCoord playerChunk = camera.getCurrentChunkCoord();
    
    if (!(playerChunk == lastPlayerChunk)) {
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