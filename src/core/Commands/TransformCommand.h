/**
 * @file TransformCommand.h
 * @brief Command for transforming scene objects with undo support
 */

#ifndef DC3D_CORE_COMMANDS_TRANSFORMCOMMAND_H
#define DC3D_CORE_COMMANDS_TRANSFORMCOMMAND_H

#include "../Command.h"

#include <QString>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>

namespace dc3d {
namespace core {

class SceneManager;

/**
 * @struct Transform
 * @brief Complete transform state (position, rotation, scale)
 */
struct Transform
{
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion
    glm::vec3 scale{1.0f};
    
    bool operator==(const Transform& other) const {
        return position == other.position 
            && rotation == other.rotation 
            && scale == other.scale;
    }
    
    bool operator!=(const Transform& other) const {
        return !(*this == other);
    }
    
    /**
     * @brief Convert to 4x4 transformation matrix
     */
    glm::mat4 toMatrix() const;
    
    /**
     * @brief Create from 4x4 transformation matrix
     */
    static Transform fromMatrix(const glm::mat4& matrix);
};

/**
 * @class TransformCommand
 * @brief Applies a transformation to a scene object
 * 
 * On execute: Applies new transform to the object
 * On undo: Restores old transform
 * 
 * Supports merging consecutive transforms of the same object
 * to avoid cluttering the undo history with tiny incremental changes.
 */
class TransformCommand : public Command
{
public:
    /**
     * @brief Construct a transform command
     * @param sceneManager The scene manager
     * @param nodeId ID of the node to transform
     * @param oldTransform Transform before the change
     * @param newTransform Transform after the change
     */
    TransformCommand(SceneManager* sceneManager, uint64_t nodeId,
                     const Transform& oldTransform, const Transform& newTransform);
    
    /**
     * @brief Construct a transform command (matrix version)
     * @param sceneManager The scene manager
     * @param nodeId ID of the node to transform
     * @param oldMatrix Transform matrix before the change
     * @param newMatrix Transform matrix after the change
     */
    TransformCommand(SceneManager* sceneManager, uint64_t nodeId,
                     const glm::mat4& oldMatrix, const glm::mat4& newMatrix);
    
    ~TransformCommand() override;
    
    void execute() override;
    void undo() override;
    QString description() const override;
    
    /**
     * @brief Check if this can merge with another transform command
     * 
     * Returns true if:
     * - Same node ID
     * - Same merge ID (for grouping incremental changes)
     */
    bool canMergeWith(const Command* other) const override;
    
    /**
     * @brief Merge another transform into this one
     * 
     * Keeps this command's old transform, takes other's new transform.
     */
    bool mergeWith(const Command* other) override;
    
    /**
     * @brief Get the node being transformed
     */
    uint64_t nodeId() const { return m_nodeId; }
    
    /**
     * @brief Set merge ID for grouping incremental transforms
     * @param id Unique ID for this transform session
     * 
     * Commands with the same merge ID and node ID will be merged.
     * Use a new ID when starting a new drag operation.
     */
    void setMergeId(int id) { m_mergeId = id; }
    
    /**
     * @brief Get the merge ID
     */
    int mergeId() const { return m_mergeId; }

private:
    SceneManager* m_sceneManager;
    uint64_t m_nodeId;
    Transform m_oldTransform;
    Transform m_newTransform;
    int m_mergeId;
    
    // For description
    enum class TransformType {
        Unknown,
        Translate,
        Rotate,
        Scale,
        Combined
    };
    TransformType detectTransformType() const;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_COMMANDS_TRANSFORMCOMMAND_H
