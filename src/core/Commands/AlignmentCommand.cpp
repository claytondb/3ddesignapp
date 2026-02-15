/**
 * @file AlignmentCommand.cpp
 * @brief Implementation of AlignmentCommand and DistributeCommand
 */

#include "AlignmentCommand.h"
#include "../SceneManager.h"
#include "geometry/MeshData.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>

namespace dc3d {
namespace core {

// ============================================================================
// AlignmentCommand
// ============================================================================

AlignmentCommand::AlignmentCommand(SceneManager* sceneManager,
                                   const std::vector<uint64_t>& meshIds,
                                   AlignAxis axis,
                                   AlignAnchor anchor)
    : Command(QStringLiteral("Align Objects"))
    , m_sceneManager(sceneManager)
    , m_meshIds(meshIds)
    , m_axis(axis)
    , m_anchor(anchor)
{
}

void AlignmentCommand::execute()
{
    if (!m_sceneManager || m_meshIds.size() < 2) {
        return;
    }
    
    // Compute transforms on first execution
    if (!m_computed) {
        computeNewTransforms();
        m_computed = true;
    }
    
    // Apply new transforms
    for (const auto& state : m_newTransforms) {
        m_sceneManager->setNodeTransform(state.meshId, state.transform);
    }
}

void AlignmentCommand::undo()
{
    if (!m_sceneManager) return;
    
    // Restore old transforms
    for (const auto& state : m_oldTransforms) {
        m_sceneManager->setNodeTransform(state.meshId, state.transform);
    }
}

QString AlignmentCommand::description() const
{
    QString axisName;
    QString anchorName;
    
    switch (m_axis) {
        case AlignAxis::X:
            axisName = "Horizontal";
            switch (m_anchor) {
                case AlignAnchor::Min: anchorName = "Left"; break;
                case AlignAnchor::Center: anchorName = "Center"; break;
                case AlignAnchor::Max: anchorName = "Right"; break;
            }
            break;
        case AlignAxis::Y:
            axisName = "Vertical";
            switch (m_anchor) {
                case AlignAnchor::Min: anchorName = "Bottom"; break;
                case AlignAnchor::Center: anchorName = "Middle"; break;
                case AlignAnchor::Max: anchorName = "Top"; break;
            }
            break;
        case AlignAxis::Z:
            axisName = "Depth";
            switch (m_anchor) {
                case AlignAnchor::Min: anchorName = "Front"; break;
                case AlignAnchor::Center: anchorName = "Center"; break;
                case AlignAnchor::Max: anchorName = "Back"; break;
            }
            break;
    }
    
    return QStringLiteral("Align %1").arg(anchorName);
}

void AlignmentCommand::computeNewTransforms()
{
    m_oldTransforms.clear();
    m_newTransforms.clear();
    
    if (m_meshIds.empty()) return;
    
    // Save old transforms and compute bounds
    float targetValue = 0.0f;
    int axisIdx = static_cast<int>(m_axis);
    
    // First pass: find the target alignment value
    bool first = true;
    for (uint64_t id : m_meshIds) {
        MeshNode* node = m_sceneManager->getMeshNode(id);
        if (!node) continue;
        
        // Save old transform (identity for now, since MeshNode doesn't store transform)
        m_oldTransforms.push_back({id, glm::mat4(1.0f)});
        
        glm::vec3 value;
        switch (m_anchor) {
            case AlignAnchor::Min:
                value = getBoundsMin(id);
                break;
            case AlignAnchor::Center:
                value = getBoundsCenter(id);
                break;
            case AlignAnchor::Max:
                value = getBoundsMax(id);
                break;
        }
        
        if (first) {
            // Use first object's position as target
            targetValue = value[axisIdx];
            first = false;
        }
    }
    
    // Second pass: compute new transforms to align to target
    for (uint64_t id : m_meshIds) {
        MeshNode* node = m_sceneManager->getMeshNode(id);
        if (!node) continue;
        
        glm::vec3 currentValue;
        switch (m_anchor) {
            case AlignAnchor::Min:
                currentValue = getBoundsMin(id);
                break;
            case AlignAnchor::Center:
                currentValue = getBoundsCenter(id);
                break;
            case AlignAnchor::Max:
                currentValue = getBoundsMax(id);
                break;
        }
        
        // Compute offset needed
        float offset = targetValue - currentValue[axisIdx];
        
        glm::vec3 translation(0.0f);
        translation[axisIdx] = offset;
        
        glm::mat4 newTransform = glm::translate(glm::mat4(1.0f), translation);
        m_newTransforms.push_back({id, newTransform});
    }
}

glm::vec3 AlignmentCommand::getBoundsMin(uint64_t meshId) const
{
    auto mesh = m_sceneManager->getMesh(meshId);
    if (!mesh) return glm::vec3(0.0f);
    
    const auto& verts = mesh->vertices();
    if (verts.empty()) return glm::vec3(0.0f);
    
    glm::vec3 minBounds = verts[0];
    for (const auto& v : verts) {
        minBounds = glm::min(minBounds, v);
    }
    return minBounds;
}

glm::vec3 AlignmentCommand::getBoundsMax(uint64_t meshId) const
{
    auto mesh = m_sceneManager->getMesh(meshId);
    if (!mesh) return glm::vec3(0.0f);
    
    const auto& verts = mesh->vertices();
    if (verts.empty()) return glm::vec3(0.0f);
    
    glm::vec3 maxBounds = verts[0];
    for (const auto& v : verts) {
        maxBounds = glm::max(maxBounds, v);
    }
    return maxBounds;
}

glm::vec3 AlignmentCommand::getBoundsCenter(uint64_t meshId) const
{
    return (getBoundsMin(meshId) + getBoundsMax(meshId)) * 0.5f;
}

// ============================================================================
// DistributeCommand
// ============================================================================

DistributeCommand::DistributeCommand(SceneManager* sceneManager,
                                     const std::vector<uint64_t>& meshIds,
                                     AlignAxis axis,
                                     bool useSpacing)
    : Command(QStringLiteral("Distribute Objects"))
    , m_sceneManager(sceneManager)
    , m_meshIds(meshIds)
    , m_axis(axis)
    , m_useSpacing(useSpacing)
{
}

void DistributeCommand::execute()
{
    if (!m_sceneManager || m_meshIds.size() < 3) {
        return;
    }
    
    if (!m_computed) {
        computeDistribution();
        m_computed = true;
    }
    
    for (const auto& state : m_newTransforms) {
        m_sceneManager->setNodeTransform(state.meshId, state.transform);
    }
}

void DistributeCommand::undo()
{
    if (!m_sceneManager) return;
    
    for (const auto& state : m_oldTransforms) {
        m_sceneManager->setNodeTransform(state.meshId, state.transform);
    }
}

QString DistributeCommand::description() const
{
    QString axisName;
    switch (m_axis) {
        case AlignAxis::X: axisName = "Horizontally"; break;
        case AlignAxis::Y: axisName = "Vertically"; break;
        case AlignAxis::Z: axisName = "Along Depth"; break;
    }
    return QStringLiteral("Distribute %1").arg(axisName);
}

void DistributeCommand::computeDistribution()
{
    m_oldTransforms.clear();
    m_newTransforms.clear();
    
    if (m_meshIds.size() < 3) return;
    
    int axisIdx = static_cast<int>(m_axis);
    
    // Collect mesh info: center position and bounds size on axis
    struct MeshInfo {
        uint64_t id;
        float center;
        float size;
        float minBound;
        float maxBound;
    };
    
    std::vector<MeshInfo> infos;
    for (uint64_t id : m_meshIds) {
        auto mesh = m_sceneManager->getMesh(id);
        if (!mesh) continue;
        
        const auto& verts = mesh->vertices();
        if (verts.empty()) continue;
        
        float minVal = std::numeric_limits<float>::max();
        float maxVal = std::numeric_limits<float>::lowest();
        for (const auto& v : verts) {
            minVal = std::min(minVal, v[axisIdx]);
            maxVal = std::max(maxVal, v[axisIdx]);
        }
        
        MeshInfo info;
        info.id = id;
        info.minBound = minVal;
        info.maxBound = maxVal;
        info.center = (minVal + maxVal) * 0.5f;
        info.size = maxVal - minVal;
        infos.push_back(info);
        
        m_oldTransforms.push_back({id, glm::mat4(1.0f)});
    }
    
    if (infos.size() < 3) return;
    
    // Sort by center position
    std::sort(infos.begin(), infos.end(),
              [](const MeshInfo& a, const MeshInfo& b) {
                  return a.center < b.center;
              });
    
    // First and last objects stay in place
    float startCenter = infos.front().center;
    float endCenter = infos.back().center;
    
    if (m_useSpacing) {
        // Distribute with equal spacing between objects
        float totalSize = 0.0f;
        for (const auto& info : infos) {
            totalSize += info.size;
        }
        
        float availableSpace = endCenter - startCenter;
        // Spacing between edges of adjacent objects
        float spacing = (availableSpace - totalSize + infos.front().size + infos.back().size) 
                       / static_cast<float>(infos.size() - 1);
        
        float currentPos = infos.front().maxBound;
        for (size_t i = 1; i < infos.size() - 1; ++i) {
            currentPos += spacing;
            float newMin = currentPos;
            float offset = newMin - infos[i].minBound;
            
            glm::vec3 translation(0.0f);
            translation[axisIdx] = offset;
            m_newTransforms.push_back({infos[i].id, glm::translate(glm::mat4(1.0f), translation)});
            
            currentPos = newMin + infos[i].size;
        }
    } else {
        // Distribute centers evenly
        float centerSpacing = (endCenter - startCenter) / static_cast<float>(infos.size() - 1);
        
        for (size_t i = 1; i < infos.size() - 1; ++i) {
            float targetCenter = startCenter + centerSpacing * static_cast<float>(i);
            float offset = targetCenter - infos[i].center;
            
            glm::vec3 translation(0.0f);
            translation[axisIdx] = offset;
            m_newTransforms.push_back({infos[i].id, glm::translate(glm::mat4(1.0f), translation)});
        }
    }
    
    // First and last don't move
    m_newTransforms.insert(m_newTransforms.begin(), {infos.front().id, glm::mat4(1.0f)});
    m_newTransforms.push_back({infos.back().id, glm::mat4(1.0f)});
}

} // namespace core
} // namespace dc3d
