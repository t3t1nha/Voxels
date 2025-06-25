#pragma once
#include "Common.h"

/**
 * Returns the view matrix representing the camera's current position and orientation.
 * @returns The 4x4 view matrix for rendering from the camera's perspective.
 */

/**
 * Updates the camera's position based on keyboard input and elapsed time.
 * @param direction Integer representing the movement direction.
 * @param deltaTime Time elapsed since the last update, used to scale movement.
 */

/**
 * Adjusts the camera's orientation based on mouse movement offsets.
 * @param xoffset Horizontal mouse movement.
 * @param yoffset Vertical mouse movement.
 */

/**
 * Returns the chunk coordinate corresponding to the camera's current position.
 * @returns The current chunk coordinate.
 */

/**
 * Recalculates the camera's directional vectors based on the current yaw and pitch angles.
 */
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