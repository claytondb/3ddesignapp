#pragma once

#include "SketchEntity.h"

namespace dc {

/**
 * @brief A reference point entity in a sketch
 * 
 * Points are used for construction geometry and as constraint references.
 * They have zero length and evaluate to the same position for all parameters.
 */
class SketchPoint : public SketchEntity {
public:
    /**
     * @brief Construct a point at the given position
     * @param position 2D position in sketch coordinates
     */
    explicit SketchPoint(const glm::dvec2& position = glm::dvec2(0.0));

    /**
     * @brief Get the point position
     */
    const glm::dvec2& getPosition() const { return m_position; }

    /**
     * @brief Set the point position
     */
    void setPosition(const glm::dvec2& position) { m_position = position; }

    // SketchEntity interface
    glm::dvec2 evaluate(double t) const override;
    glm::dvec2 getStartPoint() const override;
    glm::dvec2 getEndPoint() const override;
    glm::dvec2 getTangent(double t) const override;
    int getDegreesOfFreedom() const override;
    void getVariables(std::vector<double>& vars) const override;
    int setVariables(const std::vector<double>& vars, int offset) override;
    std::unique_ptr<SketchEntity> clone() const override;
    double getLength() const override;
    void getBoundingBox(glm::dvec2& minPt, glm::dvec2& maxPt) const override;

private:
    glm::dvec2 m_position;
};

} // namespace dc
