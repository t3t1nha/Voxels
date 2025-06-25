#include "Generation/Biomes.h"
#include "Generation/Noise.h"
#include "Common.h"

/**
 * @brief Selects a biome based on world coordinates and a seed value.
 *
 * Uses Perlin noise to deterministically choose a biome from a predefined set, ensuring spatial and seed-based variation in biome distribution.
 *
 * @param worldX The X coordinate in the world.
 * @param worldZ The Z coordinate in the world.
 * @param seed The seed value influencing biome selection.
 * @return Biome The biome corresponding to the given coordinates and seed.
 */
Biome selectBiome(int worldX, int worldZ, int seed) {
    static Biome biomes[] = {
        {"Plains", GRASS, DIRT, STONE, 20.0f, 4.0f},
        {"Mountains", SNOW, GRASS, STONE, 32.0f, 18.0f},
        {"Desert", SAND, SAND, STONE, 18.0f, 2.0f},
        {"Forest", GRASS, DIRT, STONE, 22.0f, 5.0f}
    };
    const int biomeCount = sizeof(biomes) / sizeof(Biome);
    float biomeNoise = perlinNoise(worldX * 0.001f, worldZ * 0.001f, seed);
    int biomeIndex = int((biomeNoise + 1.0f) * 0.5f * biomeCount) % biomeCount;
    return biomes[biomeIndex];
}