#include "Generation/Biomes.h"
#include "Generation/Noise.h"

Biome selectBiome(int worldX, int worldZ) {
    static Biome biomes[] = {
        {"Plains", GRASS, DIRT, STONE, 20.0f, 4.0f},
        {"Mountains", SNOW, GRASS, STONE, 32.0f, 18.0f},
        {"Desert", SAND, SAND, STONE, 18.0f, 2.0f},
        {"Forest", GRASS, DIRT, STONE, 22.0f, 5.0f}
    };
    const int biomeCount = sizeof(biomes) / sizeof(Biome);
    float biomeNoise = perlinNoise(worldX * 0.001f, worldZ * 0.001f);
    int biomeIndex = int((biomeNoise + 1.0f) * 0.5f * biomeCount) % biomeCount;
    return biomes[biomeIndex];
}