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

// Exemplo de uso para sua main.cpp
// Você deve passar os modelos e as matrizes de modelagem corretas
// Função para criar bounding boxes de todos os objetos relevantes
std::map<std::string, BoundingBox> CreateWorldBoundingBoxes(
    const std::map<std::string, ObjModel*>& loadedModels,
    const std::map<std::string, glm::mat4>& modelMatrices
) {
    std::map<std::string, BoundingBox> result;
    for (const auto& pair : loadedModels) {
        const std::string& name = pair.first;
        const ObjModel* model = pair.second;
        if (modelMatrices.count(name) == 0) continue;
        BoundingBox local = ComputeLocalBoundingBox(model->attrib);
        BoundingBox world = TransformBoundingBox(local, modelMatrices.at(name));
        result[name] = world;
    }
    return result;
}

// Exemplo de checagem de colisão entre objetos móveis e estáticos
std::vector<std::pair<std::string, std::string>> CheckCollisions(
    const std::vector<ObjectInstance>& moving,
    const std::vector<ObjectInstance>& statics
) {
    std::vector<std::pair<std::string, std::string>> collisions;
    for (const auto& mov : moving) {
        for (const auto& stat : statics) {
            if (IntersectAABB(mov.box_world, stat.box_world)) {
                collisions.emplace_back(mov.name, stat.name);
            }
        }
    }
    return collisions;
}