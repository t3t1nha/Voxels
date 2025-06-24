// Frustum.cpp
#include "Frustum.h"
#include <glm/gtc/matrix_access.hpp>

void Frustum::update(const glm::mat4& viewProj) {
    // Extract the view-projection matrix rows
    const glm::vec4 row0 = glm::row(viewProj, 0);
    const glm::vec4 row1 = glm::row(viewProj, 1);
    const glm::vec4 row2 = glm::row(viewProj, 2);
    const glm::vec4 row3 = glm::row(viewProj, 3);

    // Left plane
    planes[Left] = row3 + row0;
    // Right plane
    planes[Right] = row3 - row0;
    // Bottom plane
    planes[Bottom] = row3 + row1;
    // Top plane
    planes[Top] = row3 - row1;
    // Near plane
    planes[Near] = row3 + row2;
    // Far plane
    planes[Far] = row3 - row2;

    // Normalize all planes
    for (auto& plane : planes) {
        float length = glm::length(glm::vec3(plane));
        if (length > 0.0f) {
            plane /= length;
        }
    }

    /* Debug output (uncomment if needed)
    std::cout << "--- Frustum Planes ---\n";
    for (int i = 0; i < 6; i++) {
        std::cout << "Plane " << i << ": (" 
                  << planes[i].x << ", "
                  << planes[i].y << ", "
                  << planes[i].z << ", "
                  << planes[i].w << ")\n";
    }
    */
}

bool Frustum::isBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
    for (int i = 0; i < Plane::Count; i++) {
        const glm::vec3 positiveVertex = {
            planes[i].x > 0 ? max.x : min.x,
            planes[i].y > 0 ? max.y : min.y,
            planes[i].z > 0 ? max.z : min.z
        };

        if (glm::dot(positiveVertex, glm::vec3(planes[i])) + planes[i].w < 0) {
            return false;
        }
    }
    return true;
}
