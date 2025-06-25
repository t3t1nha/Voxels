// Frustum.h
#pragma once
#include <glm/glm.hpp>

class Frustum {
public:
    enum Plane { Left = 0, Right, Bottom, Top, Near, Far, Count };
    glm::vec4 planes[Count];

    void update(const glm::mat4& viewProj);
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
};