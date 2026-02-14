/**
 * @file AlignmentCommand.h
 * @brief Command for aligning selected objects
 * 
 * Provides undoable alignment operations:
 * - Align Left/Center/Right (X axis)
 * - Align Top/Middle/Bottom (Y axis)
 * - Distribute evenly along axis
 */

#ifndef DC3D_CORE_ALIGNMENTCOMMAND_H
#define DC3D_CORE_ALIGNMENTCOMMAND_H

#include "../Command.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace dc3d {
namespace core {

class SceneManager;

/**
 * @brief Alignment axis
 */
enum class AlignAxis {
    X,  ///< Horizontal (Left/Center/Right)
    Y,  ///< Vertical (Bottom/Middle/Top)
    Z   ///< Depth (Front/Center/Back)
};

/**
 * @brief Alignment anchor point
 */
enum class AlignAnchor {
    Min,    ///< Minimum bound (Left, Bottom, Front)
    Center, ///< Center
    Max     ///< Maximum bound (Right, Top, Back)
};

/**
 * @brief Stores transform state for undo
 */
struct ObjectTransformState {
    uint64_t meshId;
    glm::mat4 transform;
};

/**
 * @brief Command to align multiple objects
 */
class AlignmentCommand : public Command {
public:
    /**
     * @brief Create alignment command
     * @param sceneManager Scene manager for transforms
     * @param meshIds IDs of meshes to align
     * @param axis Axis to align along
     * @param anchor Alignment anchor point
     */
    AlignmentCommand(SceneManager* sceneManager,
                     const std::vector<uint64_t>& meshIds,
                     AlignAxis axis,
                     AlignAnchor anchor);
    
    ~AlignmentCommand() override = default;
    
    void execute() override;
    void undo() override;
    
    QString description() const override;
    
private:
    void computeNewTransforms();
    glm::vec3 getBoundsMin(uint64_t meshId) const;
    glm::vec3 getBoundsMax(uint64_t meshId) const;
    glm::vec3 getBoundsCenter(uint64_t meshId) const;
    
    SceneManager* m_sceneManager;
    std::vector<uint64_t> m_meshIds;
    AlignAxis m_axis;
    AlignAnchor m_anchor;
    
    std::vector<ObjectTransformState> m_oldTransforms;
    std::vector<ObjectTransformState> m_newTransforms;
    bool m_computed = false;
};

/**
 * @brief Command to distribute objects evenly
 */
class DistributeCommand : public Command {
public:
    /**
     * @brief Create distribute command
     * @param sceneManager Scene manager for transforms
     * @param meshIds IDs of meshes to distribute (order matters)
     * @param axis Axis to distribute along
     * @param useSpacing If true, distribute with equal spacing; if false, use centers
     */
    DistributeCommand(SceneManager* sceneManager,
                      const std::vector<uint64_t>& meshIds,
                      AlignAxis axis,
                      bool useSpacing = true);
    
    ~DistributeCommand() override = default;
    
    void execute() override;
    void undo() override;
    
    QString description() const override;
    
private:
    void computeDistribution();
    
    SceneManager* m_sceneManager;
    std::vector<uint64_t> m_meshIds;
    AlignAxis m_axis;
    bool m_useSpacing;
    
    std::vector<ObjectTransformState> m_oldTransforms;
    std::vector<ObjectTransformState> m_newTransforms;
    bool m_computed = false;
};

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_ALIGNMENTCOMMAND_H
