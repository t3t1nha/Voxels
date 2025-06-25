#include "Generation/Noise.h"
#include "Common.h"
#include <cmath>

/**
 * @brief Generates a deterministic pseudo-random noise value for integer coordinates and a seed.
 *
 * Produces a float in the range approximately [-1, 1] based on the input coordinates and seed, suitable for procedural terrain or texture generation.
 *
 * @param x Integer x-coordinate.
 * @param z Integer z-coordinate.
 * @param seed Seed value for noise generation.
 * @return float Pseudo-random noise value in the range [-1, 1].
 */
float noise(int x, int z, int seed) {
    int n = x + z * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

/**
 * @brief Computes a smoothed noise value at floating-point coordinates.
 *
 * Calculates a weighted average of noise values at the surrounding grid points to produce a smoother noise value at the specified (x, z) coordinates. The global seed is used for noise generation.
 *
 * @param x The x-coordinate.
 * @param z The z-coordinate.
 * @return Smoothed noise value at (x, z).
 */
float smoothNoise(float x, float z, int seed) {
    float corners = (noise(x-1, z-1, GLOBAL_SEED) + noise(x+1, z-1, GLOBAL_SEED) + noise(x-1, z+1, GLOBAL_SEED) + noise(x+1, z+1, GLOBAL_SEED)) / 16.0f;
    float sides = (noise(x-1, z, GLOBAL_SEED) + noise(x+1, z, GLOBAL_SEED) + noise(x, z-1, GLOBAL_SEED) + noise(x, z+1, GLOBAL_SEED)) / 8.0f;
    float center = noise(x, z, GLOBAL_SEED) / 4.0f;
    return corners + sides + center;
}

/**
 * @brief Computes a bilinearly interpolated noise value at floating-point coordinates.
 *
 * Calculates a smooth noise value at (x, z) by bilinearly interpolating between the smoothed noise values at the four surrounding integer grid points. The global seed is used for noise generation.
 *
 * @param x The x-coordinate.
 * @param z The z-coordinate.
 * @param seed Ignored; the global seed is used internally.
 * @return float The interpolated noise value at (x, z).
 */
float interpolatedNoise(float x, float z, int seed) {
    int intX = (int)x;
    float fracX = x - intX;
    int intZ = (int)z;
    float fracZ = z - intZ;
    
    float v1 = smoothNoise(intX, intZ, GLOBAL_SEED);
    float v2 = smoothNoise(intX + 1, intZ, GLOBAL_SEED);
    float v3 = smoothNoise(intX, intZ + 1, GLOBAL_SEED);
    float v4 = smoothNoise(intX + 1, intZ + 1, GLOBAL_SEED);
    
    float i1 = v1 * (1 - fracX) + v2 * fracX;
    float i2 = v3 * (1 - fracX) + v4 * fracX;
    
    return i1 * (1 - fracZ) + i2 * fracZ;
}

/**
 * @brief Computes Perlin noise at the specified coordinates using multiple octaves.
 *
 * Generates a smooth, continuous noise value at the given (x, z) coordinates by summing several layers (octaves) of interpolated noise, each with increasing frequency and decreasing amplitude. The global seed is used for noise generation.
 *
 * @param x The x-coordinate in noise space.
 * @param z The z-coordinate in noise space.
 * @param seed Ignored; the global seed is used internally.
 * @return float The computed Perlin noise value at (x, z).
 */
float perlinNoise(float x, float z, int seed) {
    float total = 0;
    float persistence = 0.5f;
    int octaves = 4;
    
    for (int i = 0; i < octaves; i++) {
        float frequency = pow(2, i);
        float amplitude = pow(persistence, i);
        total += interpolatedNoise(x * frequency, z * frequency, GLOBAL_SEED) * amplitude;
    }
    
    return total;
}