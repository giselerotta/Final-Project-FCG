#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/glm.hpp>
#include "tiny_obj_loader.h"
#include <string>
#include <vector>
#include "object.h"

// Estrutura para Axis-Aligned Bounding Box (AABB)
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

BoundingBox ComputeLocalBoundingBox(const tinyobj::attrib_t& attrib);
BoundingBox TransformBoundingBox(const BoundingBox& box, const glm::mat4& model);
bool IntersectAABB(const BoundingBox& a, const BoundingBox& b);
bool PointInsideAABB(const glm::vec3& point, const BoundingBox& box);
bool PointInsidePlane(const glm::vec3& point, const BoundingBox& box);

struct ObjectInstance {
    std::string name;
    BoundingBox box_world;
};

#endif // COLLISIONS_H