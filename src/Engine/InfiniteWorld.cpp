#include "Engine/InfiniteWorld.h"
#include <iostream>
/**
 * @brief Constructs an InfiniteWorld and initializes the last known player chunk coordinate to (0, 0).
 */
InfiniteWorld::InfiniteWorld() {
    lastPlayerChunk = ChunkCoord(0, 0);
}

InfiniteWorld::~InfiniteWorld() {
    for (auto& pair : chunks) {
        delete pair.second;
    }
    chunks.clear();
}

/**
 * @brief Retrieves the chunk at the specified coordinate, creating it if necessary.
 *
 * If the chunk does not exist, a new chunk is created, added to the collection, and neighboring chunks are marked as dirty.
 *
 * @param coord The coordinate of the chunk to retrieve.
 * @return Pointer to the chunk at the given coordinate.
 */
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

/**
 * @brief Ensures the chunk at the specified coordinate is loaded.
 *
 * If the chunk does not exist, it is created and added to the world.
 */
void InfiniteWorld::loadChunk(ChunkCoord coord) {
    getChunk(coord);
}

/**
 * @brief Updates the world state based on the player's current chunk position.
 *
 * If the player has moved to a new chunk, loads nearby chunks within the render distance and unloads distant chunks.
 */
void InfiniteWorld::update(const Camera& camera) {
    ChunkCoord playerChunk = camera.getCurrentChunkCoord();
    
    if (!(playerChunk == lastPlayerChunk)) {
        lastPlayerChunk = playerChunk;
        loadChunksAroundPlayer(playerChunk);
        unloadDistantChunks(playerChunk);
    }
}

/**
 * @brief Loads all chunks within the render distance around the specified player chunk.
 *
 * Ensures that every chunk within a square area centered on the player's current chunk coordinate and extending `RENDER_DISTANCE` units in each direction is loaded. Missing chunks are created and loaded as needed.
 *
 * @param playerChunk The chunk coordinate representing the player's current location.
 */
void InfiniteWorld::loadChunksAroundPlayer(ChunkCoord playerChunk) {
    for (int x = playerChunk.x - RENDER_DISTANCE; x <= playerChunk.x + RENDER_DISTANCE; x++) {
        for (int z = playerChunk.z - RENDER_DISTANCE; z <= playerChunk.z + RENDER_DISTANCE; z++) {
            ChunkCoord coord(x, z);
            if (chunks.find(coord) == chunks.end()) {
                loadChunk(coord);
            }
        }
    }
}

/**
 * @brief Unloads chunks that are outside the allowed distance from the player.
 *
 * Identifies and removes all chunks whose coordinates are farther than `RENDER_DISTANCE + 2` from the player's current chunk position. Frees associated memory and removes them from the internal chunk map.
 */
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
            Chunk* chunk = it->second;
                delete chunk;
                std::cout << "Unloaded chunk at (" << coord.x << ", " << coord.z << ")" << std::endl;
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

/**
 * @brief Determines if the voxel at the specified world coordinates is solid (active).
 *
 * Converts world coordinates to the corresponding chunk and local voxel coordinates. Returns false if the chunk is not loaded or the coordinates are out of bounds; otherwise, returns true if the voxel is active (solid).
 *
 * @param worldX The X coordinate in world space.
 * @param worldY The Y coordinate in world space.
 * @param worldZ The Z coordinate in world space.
 * @return true if the voxel is solid (active); false otherwise.
 */
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

/**
 * @brief Sets the voxel type at the specified world coordinates.
 *
 * Updates the voxel at the given world position if the corresponding chunk is loaded and the coordinates are within bounds. Marks the affected chunk and its neighbors for mesh updates.
 *
 * @param worldX The X coordinate in world space.
 * @param worldY The Y coordinate in world space.
 * @param worldZ The Z coordinate in world space.
 * @param type The voxel type to set at the specified location.
 */
void InfiniteWorld::setVoxel(int worldX, int worldY, int worldZ, VoxelType type) {
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    ChunkCoord coord(chunkX, chunkZ);
    auto it = chunks.find(coord);
    if (it == chunks.end()) {
        return; // Chunk not loaded
    }

    Chunk* chunk = it->second;

    int localX = worldX - (chunkX * CHUNK_SIZE);
    int localY = worldY;
    int localZ = worldZ - (chunkZ * CHUNK_SIZE);

    if (localX < 0 || localX >= CHUNK_SIZE ||
        localY < 0 || localY >= CHUNK_HEIGHT || 
        localZ < 0 || localZ >= CHUNK_SIZE) {
        return; // Out of bounds
    }

    chunk->voxels[localX][localY][localZ] = Voxel(type);
    chunk->meshDirty = true;
    markNeighbourChunksDirty(coord);
}

    /**
 * @brief Marks the mesh of neighboring chunks as dirty.
 *
 * Sets the `meshDirty` flag to true for each of the four adjacent chunks (left, right, front, back) of the specified chunk coordinate, if those chunks are currently loaded.
 *
 * @param coord The coordinate of the chunk whose neighbors will be marked dirty.
 */
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

/**
 * @brief Returns the voxel type at the specified world coordinates.
 *
 * If the corresponding chunk is not loaded, the chunk pointer is null, or the coordinates are out of bounds, returns AIR. Otherwise, returns the type of the voxel if it is active; returns AIR if inactive.
 *
 * @param worldX The X coordinate in world space.
 * @param worldY The Y coordinate in world space.
 * @param worldZ The Z coordinate in world space.
 * @return VoxelType The type of the voxel at the given coordinates, or AIR if not present or inactive.
 */
VoxelType InfiniteWorld::getVoxelTypeAt(int worldX, int worldY, int worldZ) {
    int chunkX = (int)floor((float)worldX / CHUNK_SIZE);
    int chunkZ = (int)floor((float)worldZ / CHUNK_SIZE);

    ChunkCoord coord(chunkX, chunkZ);
    auto it = chunks.find(coord);
    if (it == chunks.end()) {
        // Debug print
        // std::cerr << "Chunk not found for coord: " << coord.x << "," << coord.z << std::endl;
        return AIR;
    }

    Chunk* chunk = it->second;
    if (!chunk) {
        // Debug print
        // std::cerr << "Null chunk pointer for coord: " << coord.x << "," << coord.z << std::endl;
        return AIR;
    }

    int localX = worldX - (chunkX * CHUNK_SIZE);
    int localY = worldY;
    int localZ = worldZ - (chunkZ * CHUNK_SIZE);

    if (localX < 0 || localX >= CHUNK_SIZE ||
        localY < 0 || localY >= CHUNK_HEIGHT ||
        localZ < 0 || localZ >= CHUNK_SIZE) {
        // Debug print
        // std::cerr << "Out of bounds voxel: " << localX << "," << localY << "," << localZ << std::endl;
        return AIR;
    }

    return chunk->voxels[localX][localY][localZ].isActive ?
           chunk->voxels[localX][localY][localZ].type : AIR;
}