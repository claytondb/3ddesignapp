#pragma once

#include "Sketch.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace dc {
namespace sketch {

/**
 * @brief Types of geometric constraints
 */
enum class ConstraintType {
    Coincident,      // Two points are at the same location
    Parallel,        // Two lines are parallel
    Perpendicular,   // Two lines are perpendicular
    Tangent,         // Two curves are tangent
    Horizontal,      // A line is horizontal
    Vertical,        // A line is vertical
    Distance,        // Distance between two points or point-to-line
    Angle,           // Angle between two lines
    Radius,          // Circle or arc radius
    Equal,           // Two segments have equal length
    Concentric,      // Two circles/arcs share center
    Midpoint,        // Point is at midpoint of line
    Symmetric,       // Two points symmetric about a line
    FixedPoint,      // Point is fixed at a location
    FixedAngle,      // Line is at fixed angle
    PointOnCurve     // Point lies on a curve
};

/**
 * @brief Reference to a geometric element in a constraint
 */
struct ConstraintRef {
    uint64_t entityId = 0;
    int pointIndex = -1;  // -1 = whole entity, 0 = start, 1 = end, 2 = center
    
    bool isPoint() const { return pointIndex >= 0; }
    bool isEntity() const { return pointIndex < 0; }
};

/**
 * @brief A geometric constraint between sketch entities
 */
class Constraint {
public:
    using Ptr = std::shared_ptr<Constraint>;
    using ConstPtr = std::shared_ptr<const Constraint>;
    
    Constraint(ConstraintType type);
    
    uint64_t getId() const { return m_id; }
    ConstraintType getType() const { return m_type; }
    
    // References to constrained entities
    std::vector<ConstraintRef> refs;
    
    // Constraint value (for dimensional constraints)
    float value = 0.0f;
    
    // Whether constraint is driving or driven
    bool isDriving = true;
    
    // Check if constraint is satisfied
    bool isSatisfied(const Sketch& sketch, float tolerance = 1e-5f) const;
    
    // Get the constraint error
    float getError(const Sketch& sketch) const;
    
    static Ptr create(ConstraintType type);

private:
    uint64_t m_id;
    ConstraintType m_type;
    
    static uint64_t s_nextId;
};

/**
 * @brief Solver status after solve attempt
 */
enum class SolverStatus {
    Success,            // Converged to solution
    UnderConstrained,   // Solution found but DOF > 0
    OverConstrained,    // Redundant or conflicting constraints
    NotConverged,       // Failed to converge
    InvalidInput        // Invalid sketch or constraints
};

/**
 * @brief Result of constraint solving
 */
struct SolveResult {
    SolverStatus status = SolverStatus::InvalidInput;
    int iterations = 0;
    float finalError = 0.0f;
    int degreesOfFreedom = 0;
    std::vector<uint64_t> conflictingConstraints;
};

/**
 * @brief Constraint solver using Newton-Raphson method
 * 
 * Solves geometric constraint systems by formulating them as
 * a system of nonlinear equations and using Newton-Raphson
 * iteration to find the solution.
 */
class SketchSolver {
public:
    using Ptr = std::shared_ptr<SketchSolver>;
    
    SketchSolver();
    ~SketchSolver() = default;
    
    /**
     * @brief Set the sketch to solve
     */
    void setSketch(Sketch::Ptr sketch);
    
    /**
     * @brief Add a constraint
     * @return Constraint ID
     */
    uint64_t addConstraint(Constraint::Ptr constraint);
    
    /**
     * @brief Remove a constraint
     */
    bool removeConstraint(uint64_t constraintId);
    
    /**
     * @brief Get a constraint by ID
     */
    Constraint::Ptr getConstraint(uint64_t constraintId);
    
    /**
     * @brief Get all constraints
     */
    const std::vector<Constraint::Ptr>& getConstraints() const { return m_constraints; }
    
    /**
     * @brief Clear all constraints
     */
    void clearConstraints();
    
    /**
     * @brief Solve the constraint system
     * Modifies entity positions to satisfy constraints
     */
    SolveResult solve();
    
    /**
     * @brief Get current degrees of freedom
     */
    int getDOF() const;
    
    /**
     * @brief Check if system is fully constrained
     */
    bool isFullyConstrained() const { return getDOF() == 0; }
    
    /**
     * @brief Set maximum iterations
     */
    void setMaxIterations(int maxIter) { m_maxIterations = maxIter; }
    
    /**
     * @brief Set convergence tolerance
     */
    void setTolerance(float tol) { m_tolerance = tol; }
    
    // ==================== Convenience Constraint Creation ====================
    
    /**
     * @brief Add coincident constraint between two points
     */
    uint64_t addCoincident(uint64_t entity1, int point1, uint64_t entity2, int point2);
    
    /**
     * @brief Add horizontal constraint to a line
     */
    uint64_t addHorizontal(uint64_t lineId);
    
    /**
     * @brief Add vertical constraint to a line
     */
    uint64_t addVertical(uint64_t lineId);
    
    /**
     * @brief Add parallel constraint between two lines
     */
    uint64_t addParallel(uint64_t line1Id, uint64_t line2Id);
    
    /**
     * @brief Add perpendicular constraint between two lines
     */
    uint64_t addPerpendicular(uint64_t line1Id, uint64_t line2Id);
    
    /**
     * @brief Add distance constraint between two points
     */
    uint64_t addDistance(uint64_t entity1, int point1, uint64_t entity2, int point2, float distance);
    
    /**
     * @brief Add angle constraint between two lines
     */
    uint64_t addAngle(uint64_t line1Id, uint64_t line2Id, float angleRadians);
    
    /**
     * @brief Add radius constraint to circle or arc
     */
    uint64_t addRadius(uint64_t entityId, float radius);
    
    /**
     * @brief Add tangent constraint between two curves
     */
    uint64_t addTangent(uint64_t entity1Id, uint64_t entity2Id);
    
    /**
     * @brief Add fixed point constraint
     */
    uint64_t addFixedPoint(uint64_t entityId, int pointIndex, const glm::vec2& position);
    
    static Ptr create();

private:
    Sketch::Ptr m_sketch;
    std::vector<Constraint::Ptr> m_constraints;
    std::unordered_map<uint64_t, size_t> m_constraintIndex;
    
    int m_maxIterations = 100;
    float m_tolerance = 1e-6f;
    
    /**
     * @brief Build the system of equations
     * @return Number of equations
     */
    int buildEquations(std::vector<float>& residuals);
    
    /**
     * @brief Build the Jacobian matrix
     */
    void buildJacobian(std::vector<std::vector<float>>& jacobian, int numEqs, int numVars);
    
    /**
     * @brief Get all variable values from sketch
     */
    std::vector<float> getVariables() const;
    
    /**
     * @brief Set variable values back to sketch
     */
    void setVariables(const std::vector<float>& vars);
    
    /**
     * @brief Get number of variables (DOF before constraints)
     */
    int getNumVariables() const;
    
    /**
     * @brief Solve linear system J * dx = -residuals
     */
    bool solveLinearSystem(const std::vector<std::vector<float>>& J, 
                          const std::vector<float>& residuals,
                          std::vector<float>& dx);
    
    /**
     * @brief Evaluate a single constraint's contribution to residuals
     */
    void evaluateConstraint(const Constraint& constraint, std::vector<float>& residuals, int& eqIndex);
    
    /**
     * @brief Get point position from entity reference
     */
    glm::vec2 getPointFromRef(const ConstraintRef& ref) const;
    
    /**
     * @brief Rebuild constraint index
     */
    void rebuildIndex();
};

} // namespace sketch
} // namespace dc
