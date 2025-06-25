// Frustum.h
#pragma once
#include <glm/glm.hpp>
#include <array>

class Frustum {
public:
    enum Plane { Left = 0, Right, Bottom, Top, Near, Far, Count };
    
    void update(const glm::mat4& viewProj);
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;

private:
    std::array<glm::vec4, Plane::Count> planes;
};