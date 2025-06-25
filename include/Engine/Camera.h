#pragma once
#include "Common.h"

class Camera {
public:
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;
    
    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    
    Camera(vec3 pos = vec3(0.0f, 32.0f, 0.0f));
    mat4 getViewMatrix();
    void processKeyboard(int direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
    ChunkCoord getCurrentChunkCoord() const;
    
private:
    void updateCameraVectors();
};