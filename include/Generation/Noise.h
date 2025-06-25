#pragma once

float noise(int x, int z, int seed);
float smoothNoise(float x, float z);
float interpolatedNoise(float x, float z);
float perlinNoise(float x, float z);