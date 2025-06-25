#include "Engine/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera(vec3 pos) {
    position = pos;
    worldUp = vec3(0.0f, 1.0f, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    movementSpeed = 15.0f;
    mouseSensitivity = 0.1f;
    updateCameraVectors();
}

mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (direction == 0) position += front * velocity; // W
    if (direction == 1) position -= front * velocity; // S
    if (direction == 2) position -= right * velocity; // A
    if (direction == 3) position += right * velocity; // D
    if (direction == 4) position += up * velocity;    // Space
    if (direction == 5) position -= up * velocity;    // Shift
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}

ChunkCoord Camera::getCurrentChunkCoord() const {
    int chunkX = (int)floor(position.x / CHUNK_SIZE);
    int chunkZ = (int)floor(position.z / CHUNK_SIZE);
    return ChunkCoord(chunkX, chunkZ);
}

void Camera::updateCameraVectors() {
    vec3 front_;
    front_.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front_.y = sin(glm::radians(pitch));
    front_.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front_);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}