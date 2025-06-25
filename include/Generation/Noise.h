#pragma once

float noise(int x, int z, int seed);
float smoothNoise(float x, float z, int seed);
float interpolatedNoise(float x, float z, int seed);
float perlinNoise(float x, float z, int seed);