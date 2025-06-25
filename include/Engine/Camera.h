#pragma once
#include "Common.h"

class Camera {
public:
    vec3 position, front, up, right, worldUp;
    float yaw, pitch;
    float movementSpeed, mouseSensitivity;

    Camera(vec3 pos = vec3(0, 50, 0));
    mat4 getViewMatrix();
    void processKeyboard(int direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
    ChunkCoord getCurrentChunkCoord() const;
    void updateCameraVectors();
};