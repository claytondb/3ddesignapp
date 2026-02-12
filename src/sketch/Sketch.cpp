#include "Sketch.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <set>

namespace dc {
namespace sketch {

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

// ==================== SketchPlane Implementation ====================

SketchPlane SketchPlane::fromOriginNormal(const glm::vec3& origin, const glm::vec3& normal) {
    SketchPlane plane;
    plane.origin = origin;
    plane.normal = glm::normalize(normal);
    
    // Compute orthonormal basis
    // Choose a vector not parallel to normal
    glm::vec3 up = std::abs(plane.normal.y) < 0.9f ? 
                   glm::vec3(0.0f, 1.0f, 0.0f) : 
                   glm::vec3(1.0f, 0.0f, 0.0f);
    
    plane.xAxis = glm::normalize(glm::cross(up, plane.normal));
    plane.yAxis = glm::cross(plane.normal, plane.xAxis);
    
    return plane;
}

SketchPlane SketchPlane::XY(float zOffset) {
    SketchPlane plane;
    plane.origin = glm::vec3(0.0f, 0.0f, zOffset);
    plane.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    plane.xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    plane.yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    return plane;
}

SketchPlane SketchPlane::XZ(float yOffset) {
    SketchPlane plane;
    plane.origin = glm::vec3(0.0f, yOffset, 0.0f);
    plane.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    plane.xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    plane.yAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    return plane;
}

SketchPlane SketchPlane::YZ(float xOffset) {
    SketchPlane plane;
    plane.origin = glm::vec3(xOffset, 0.0f, 0.0f);
    plane.normal = glm::vec3(1.0f, 0.0f, 0.0f);
    plane.xAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    plane.yAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    return plane;
}

glm::vec3 SketchPlane::toWorld(const glm::vec2& local) const {
    return origin + local.x * xAxis + local.y * yAxis;
}

glm::vec2 SketchPlane::toLocal(const glm::vec3& world) const {
    glm::vec3 offset = world - origin;
    return glm::vec2(glm::dot(offset, xAxis), glm::dot(offset, yAxis));
}

glm::mat4 SketchPlane::getTransformMatrix() const {
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(xAxis, 0.0f);
    m[1] = glm::vec4(yAxis, 0.0f);
    m[2] = glm::vec4(normal, 0.0f);
    m[3] = glm::vec4(origin, 1.0f);
    return m;
}

glm::mat4 SketchPlane::getInverseTransformMatrix() const {
    // For orthonormal basis, inverse is just transpose of rotation part
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(xAxis.x, yAxis.x, normal.x, 0.0f);
    m[1] = glm::vec4(xAxis.y, yAxis.y, normal.y, 0.0f);
    m[2] = glm::vec4(xAxis.z, yAxis.z, normal.z, 0.0f);
    glm::vec3 t(-glm::dot(origin, xAxis), -glm::dot(origin, yAxis), -glm::dot(origin, normal));
    m[3] = glm::vec4(t, 1.0f);
    return m;
}

bool SketchPlane::containsPoint(const glm::vec3& point, float tolerance) const {
    return std::abs(distanceToPoint(point)) <= tolerance;
}

float SketchPlane::distanceToPoint(const glm::vec3& point) const {
    return glm::dot(point - origin, normal);
}

glm::vec3 SketchPlane::projectPoint(const glm::vec3& point) const {
    float d = distanceToPoint(point);
    return point - d * normal;
}

// ==================== Sketch Implementation ====================

uint64_t Sketch::s_nextId = 1;

Sketch::Sketch(const SketchPlane& plane)
    : m_id(s_nextId++)
    , m_name("Sketch" + std::to_string(m_id))
    , m_plane(plane)
{
}

uint64_t Sketch::addEntity(SketchEntity::Ptr entity) {
    if (!entity) {
        return 0;
    }
    
    uint64_t id = entity->getId();
    m_entityIndex[id] = m_entities.size();
    m_entities.push_back(std::move(entity));
    return id;
}

bool Sketch::removeEntity(uint64_t entityId) {
    auto it = m_entityIndex.find(entityId);
    if (it == m_entityIndex.end()) {
        return false;
    }
    
    size_t index = it->second;
    m_entities.erase(m_entities.begin() + index);
    rebuildIndex();
    return true;
}

SketchEntity::Ptr Sketch::getEntity(uint64_t entityId) {
    auto it = m_entityIndex.find(entityId);
    if (it == m_entityIndex.end()) {
        return nullptr;
    }
    return m_entities[it->second];
}

SketchEntity::ConstPtr Sketch::getEntity(uint64_t entityId) const {
    auto it = m_entityIndex.find(entityId);
    if (it == m_entityIndex.end()) {
        return nullptr;
    }
    return m_entities[it->second];
}

std::vector<SketchEntity::Ptr> Sketch::getEntitiesByType(SketchEntityType type) const {
    std::vector<SketchEntity::Ptr> result;
    for (const auto& entity : m_entities) {
        if (entity->getType() == type) {
            result.push_back(entity);
        }
    }
    return result;
}

void Sketch::clearEntities() {
    m_entities.clear();
    m_entityIndex.clear();
}

SketchLine::Ptr Sketch::addLine(const glm::vec2& start, const glm::vec2& end) {
    auto line = SketchLine::create(start, end);
    addEntity(line);
    return line;
}

SketchArc::Ptr Sketch::addArc(const glm::vec2& center, float radius, float startAngle, float endAngle) {
    auto arc = SketchArc::create(center, radius, startAngle, endAngle);
    addEntity(arc);
    return arc;
}

SketchCircle::Ptr Sketch::addCircle(const glm::vec2& center, float radius) {
    auto circle = SketchCircle::create(center, radius);
    addEntity(circle);
    return circle;
}

SketchSpline::Ptr Sketch::addSpline(const std::vector<glm::vec2>& controlPoints, int degree) {
    auto spline = SketchSpline::create(controlPoints, degree);
    addEntity(spline);
    return spline;
}

std::vector<SketchLine::Ptr> Sketch::addRectangle(const glm::vec2& corner1, const glm::vec2& corner2) {
    std::vector<SketchLine::Ptr> lines;
    
    glm::vec2 p0 = corner1;
    glm::vec2 p1(corner2.x, corner1.y);
    glm::vec2 p2 = corner2;
    glm::vec2 p3(corner1.x, corner2.y);
    
    lines.push_back(addLine(p0, p1));
    lines.push_back(addLine(p1, p2));
    lines.push_back(addLine(p2, p3));
    lines.push_back(addLine(p3, p0));
    
    return lines;
}

std::vector<SketchLine::Ptr> Sketch::addPolygon(const glm::vec2& center, float radius, 
                                                  int sides, float startAngle) {
    std::vector<SketchLine::Ptr> lines;
    
    if (sides < 3) {
        return lines;
    }
    
    std::vector<glm::vec2> vertices;
    float angleStep = TWO_PI / static_cast<float>(sides);
    
    for (int i = 0; i < sides; ++i) {
        float angle = startAngle + i * angleStep;
        vertices.emplace_back(center + radius * glm::vec2(std::cos(angle), std::sin(angle)));
    }
    
    for (int i = 0; i < sides; ++i) {
        lines.push_back(addLine(vertices[i], vertices[(i + 1) % sides]));
    }
    
    return lines;
}

void Sketch::selectEntity(uint64_t entityId) {
    if (auto entity = getEntity(entityId)) {
        entity->setSelected(true);
    }
}

void Sketch::deselectEntity(uint64_t entityId) {
    if (auto entity = getEntity(entityId)) {
        entity->setSelected(false);
    }
}

void Sketch::toggleEntitySelection(uint64_t entityId) {
    if (auto entity = getEntity(entityId)) {
        entity->setSelected(!entity->isSelected());
    }
}

void Sketch::clearSelection() {
    for (auto& entity : m_entities) {
        entity->setSelected(false);
    }
}

std::vector<SketchEntity::Ptr> Sketch::getSelectedEntities() const {
    std::vector<SketchEntity::Ptr> selected;
    for (const auto& entity : m_entities) {
        if (entity->isSelected()) {
            selected.push_back(entity);
        }
    }
    return selected;
}

BoundingBox2D Sketch::boundingBox() const {
    BoundingBox2D box;
    for (const auto& entity : m_entities) {
        box.expand(entity->boundingBox());
    }
    return box;
}

SketchEntity::Ptr Sketch::findNearestEntity(const glm::vec2& point, float maxDistance) const {
    SketchEntity::Ptr nearest = nullptr;
    float minDist = maxDistance;
    
    for (const auto& entity : m_entities) {
        float t = entity->closestParameter(point);
        glm::vec2 closest = entity->evaluate(t);
        float dist = glm::length(point - closest);
        
        if (dist < minDist) {
            minDist = dist;
            nearest = entity;
        }
    }
    
    return nearest;
}

std::vector<SketchEntity::Ptr> Sketch::findEntitiesInBox(const BoundingBox2D& box) const {
    std::vector<SketchEntity::Ptr> result;
    
    for (const auto& entity : m_entities) {
        BoundingBox2D entityBox = entity->boundingBox();
        
        // Check for overlap
        if (entityBox.max.x >= box.min.x && entityBox.min.x <= box.max.x &&
            entityBox.max.y >= box.min.y && entityBox.min.y <= box.max.y) {
            result.push_back(entity);
        }
    }
    
    return result;
}

glm::vec2 Sketch::worldToLocal(const glm::vec3& worldPoint) const {
    return m_plane.toLocal(worldPoint);
}

glm::vec3 Sketch::localToWorld(const glm::vec2& localPoint) const {
    return m_plane.toWorld(localPoint);
}

bool Sketch::hasClosed() const {
    return !findClosedLoops().empty();
}

std::vector<std::vector<uint64_t>> Sketch::findClosedLoops() const {
    std::vector<std::vector<uint64_t>> loops;
    
    // Build connectivity graph
    // For each entity endpoint, find other entities that share that point
    const float tolerance = 1e-5f;
    
    // Get start/end points for each entity
    struct Endpoint {
        uint64_t entityId;
        glm::vec2 point;
        bool isStart;
    };
    
    std::vector<Endpoint> endpoints;
    
    for (const auto& entity : m_entities) {
        if (entity->isConstruction()) continue;
        
        // Skip circles (they're already closed)
        if (entity->getType() == SketchEntityType::Circle) {
            loops.push_back({entity->getId()});
            continue;
        }
        
        endpoints.push_back({entity->getId(), entity->evaluate(0.0f), true});
        endpoints.push_back({entity->getId(), entity->evaluate(1.0f), false});
    }
    
    // Build adjacency: for each endpoint, find connected endpoints
    std::unordered_map<uint64_t, std::vector<uint64_t>> adjacency;
    
    for (size_t i = 0; i < endpoints.size(); ++i) {
        for (size_t j = i + 1; j < endpoints.size(); ++j) {
            if (endpoints[i].entityId == endpoints[j].entityId) continue;
            
            if (glm::length(endpoints[i].point - endpoints[j].point) < tolerance) {
                adjacency[endpoints[i].entityId].push_back(endpoints[j].entityId);
                adjacency[endpoints[j].entityId].push_back(endpoints[i].entityId);
            }
        }
    }
    
    // Find loops using DFS
    std::set<uint64_t> visited;
    
    for (const auto& entity : m_entities) {
        if (entity->isConstruction()) continue;
        if (entity->getType() == SketchEntityType::Circle) continue;
        if (visited.count(entity->getId())) continue;
        
        // Try to find a loop starting from this entity
        std::vector<uint64_t> path;
        path.push_back(entity->getId());
        
        std::function<bool(uint64_t, uint64_t)> findLoop = [&](uint64_t current, uint64_t start) -> bool {
            if (path.size() > 2 && current == start) {
                return true;
            }
            
            auto it = adjacency.find(current);
            if (it == adjacency.end()) return false;
            
            for (uint64_t next : it->second) {
                if (next == start && path.size() > 2) {
                    return true;
                }
                if (std::find(path.begin(), path.end(), next) != path.end()) continue;
                
                path.push_back(next);
                if (findLoop(next, start)) {
                    return true;
                }
                path.pop_back();
            }
            
            return false;
        };
        
        if (findLoop(entity->getId(), entity->getId()) && path.size() >= 3) {
            loops.push_back(path);
            for (uint64_t id : path) {
                visited.insert(id);
            }
        }
    }
    
    return loops;
}

void Sketch::rebuildIndex() {
    m_entityIndex.clear();
    for (size_t i = 0; i < m_entities.size(); ++i) {
        m_entityIndex[m_entities[i]->getId()] = i;
    }
}

Sketch::Ptr Sketch::create(const SketchPlane& plane) {
    return std::make_shared<Sketch>(plane);
}

} // namespace sketch
} // namespace dc
