/**
 * @file TransformCommand.cpp
 * @brief Implementation of TransformCommand
 */

#include "TransformCommand.h"
#include "../SceneManager.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace dc3d {
namespace core {

// ============================================================================
// Transform
// ============================================================================

glm::mat4 Transform::toMatrix() const
{
    glm::mat4 result = glm::mat4(1.0f);
    
    // Apply scale
    result = glm::scale(result, scale);
    
    // Apply rotation
    result = glm::mat4_cast(rotation) * result;
    
    // Apply translation
    result[3] = glm::vec4(position, 1.0f);
    
    return result;
}

Transform Transform::fromMatrix(const glm::mat4& matrix)
{
    Transform t;
    
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, t.scale, t.rotation, t.position, skew, perspective);
    
    return t;
}

// ============================================================================
// TransformCommand
// ============================================================================

TransformCommand::TransformCommand(SceneManager* sceneManager, uint64_t nodeId,
                                   const Transform& oldTransform, const Transform& newTransform)
    : m_sceneManager(sceneManager)
    , m_nodeId(nodeId)
    , m_oldTransform(oldTransform)
    , m_newTransform(newTransform)
    , m_mergeId(-1)  // No merging by default
{
}

TransformCommand::TransformCommand(SceneManager* sceneManager, uint64_t nodeId,
                                   const glm::mat4& oldMatrix, const glm::mat4& newMatrix)
    : m_sceneManager(sceneManager)
    , m_nodeId(nodeId)
    , m_oldTransform(Transform::fromMatrix(oldMatrix))
    , m_newTransform(Transform::fromMatrix(newMatrix))
    , m_mergeId(-1)
{
}

TransformCommand::~TransformCommand() = default;

void TransformCommand::execute()
{
    if (!m_sceneManager) {
        return;
    }
    
    m_sceneManager->setNodeTransform(m_nodeId, m_newTransform.toMatrix());
}

void TransformCommand::undo()
{
    if (!m_sceneManager) {
        return;
    }
    
    m_sceneManager->setNodeTransform(m_nodeId, m_oldTransform.toMatrix());
}

QString TransformCommand::description() const
{
    TransformType type = detectTransformType();
    
    switch (type) {
        case TransformType::Translate:
            return QStringLiteral("Move");
        case TransformType::Rotate:
            return QStringLiteral("Rotate");
        case TransformType::Scale:
            return QStringLiteral("Scale");
        case TransformType::Combined:
        case TransformType::Unknown:
        default:
            return QStringLiteral("Transform");
    }
}

bool TransformCommand::canMergeWith(const Command* other) const
{
    // Only merge if we have a valid merge ID
    if (m_mergeId < 0) {
        return false;
    }
    
    const TransformCommand* otherTransform = dynamic_cast<const TransformCommand*>(other);
    if (!otherTransform) {
        return false;
    }
    
    // Must be same node and same merge session
    return m_nodeId == otherTransform->m_nodeId 
        && m_mergeId == otherTransform->m_mergeId;
}

bool TransformCommand::mergeWith(const Command* other)
{
    const TransformCommand* otherTransform = dynamic_cast<const TransformCommand*>(other);
    if (!otherTransform) {
        return false;
    }
    
    // Keep our old transform, take their new transform
    // This collapses multiple incremental changes into one
    m_newTransform = otherTransform->m_newTransform;
    
    return true;
}

TransformCommand::TransformType TransformCommand::detectTransformType() const
{
    const float epsilon = 1e-5f;
    
    bool positionChanged = glm::length(m_newTransform.position - m_oldTransform.position) > epsilon;
    bool rotationChanged = glm::abs(glm::dot(m_newTransform.rotation, m_oldTransform.rotation) - 1.0f) > epsilon;
    bool scaleChanged = glm::length(m_newTransform.scale - m_oldTransform.scale) > epsilon;
    
    int changeCount = (positionChanged ? 1 : 0) + (rotationChanged ? 1 : 0) + (scaleChanged ? 1 : 0);
    
    if (changeCount == 0) {
        return TransformType::Unknown;
    }
    if (changeCount > 1) {
        return TransformType::Combined;
    }
    
    if (positionChanged) return TransformType::Translate;
    if (rotationChanged) return TransformType::Rotate;
    if (scaleChanged) return TransformType::Scale;
    
    return TransformType::Unknown;
}

} // namespace core
} // namespace dc3d
