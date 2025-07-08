#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <tiny_obj_loader.h>
#include "object.h"
#include "collisions.h"

// Calcula a bounding box local de um modelo tinyobj
BoundingBox ComputeLocalBoundingBox(const tinyobj::attrib_t& attrib) {
    BoundingBox box;
    box.min = glm::vec3(std::numeric_limits<float>::max());
    box.max = glm::vec3(std::numeric_limits<float>::lowest());
    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        glm::vec3 v(attrib.vertices[i], attrib.vertices[i+1], attrib.vertices[i+2]);
        box.min = glm::min(box.min, v);
        box.max = glm::max(box.max, v);
    }
    return box;
}

// Transforma a bounding box local para o mundo usando a model matrix
BoundingBox TransformBoundingBox(const BoundingBox& box, const glm::mat4& model) {
    glm::vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z},
        {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z},
        {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z},
        {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z},
        {box.max.x, box.max.y, box.max.z}
    };
    glm::vec3 new_min = glm::vec3(model * glm::vec4(corners[0], 1.0f));
    glm::vec3 new_max = new_min;
    for (int i = 1; i < 8; ++i) {
        glm::vec3 transformed = glm::vec3(model * glm::vec4(corners[i], 1.0f));
        new_min = glm::min(new_min, transformed);
        new_max = glm::max(new_max, transformed);
    }
    return {new_min, new_max};
}

// Checa interseção entre duas AABBs
bool IntersectAABB(const BoundingBox& a, const BoundingBox& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool PointInsideAABB(const glm::vec3& point, const BoundingBox& box) {
    return (point.x >= box.min.x && point.x <= box.max.x) &&
           (point.y >= box.min.y && point.y <= box.max.y) &&
           (point.z >= box.min.z && point.z <= box.max.z);
}
