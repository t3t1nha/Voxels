#include "Generation/Noise.h"
#include "Common.h"
#include <cmath>

float noise(int x, int z, int seed) {
    int n = x + z * 57 + seed * 131;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float smoothNoise(float x, float z, int seed) {
    float corners = (noise(x-1, z-1, GLOBAL_SEED) + noise(x+1, z-1, GLOBAL_SEED) + noise(x-1, z+1, GLOBAL_SEED) + noise(x+1, z+1, GLOBAL_SEED)) / 16.0f;
    float sides = (noise(x-1, z, GLOBAL_SEED) + noise(x+1, z, GLOBAL_SEED) + noise(x, z-1, GLOBAL_SEED) + noise(x, z+1, GLOBAL_SEED)) / 8.0f;
    float center = noise(x, z, GLOBAL_SEED) / 4.0f;
    return corners + sides + center;
}

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