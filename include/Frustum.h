// Frustum.h
#pragma once
#include <glm/glm.hpp>
#include <array>

/**
 * Updates the frustum planes based on the provided view-projection matrix.
 * 
 * Extracts and normalizes the six frustum planes from the combined view-projection matrix to define the current frustum boundaries.
 * @param viewProj Combined view-projection matrix used to update the frustum.
 */

/**
 * Determines whether an axis-aligned bounding box is at least partially inside the frustum.
 *
 * Returns true if the box defined by its minimum and maximum 3D coordinates intersects or is contained within the frustum; otherwise, returns false.
 * @param min Minimum 3D coordinates of the bounding box.
 * @param max Maximum 3D coordinates of the bounding box.
 * @return True if the box is visible within the frustum, false otherwise.
 */
class Frustum {
public:
    enum Plane { Left = 0, Right, Bottom, Top, Near, Far, Count };
    
    void update(const glm::mat4& viewProj);
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;

private:
    std::array<glm::vec4, Plane::Count> planes;
};