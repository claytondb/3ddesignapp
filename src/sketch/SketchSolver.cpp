#include "SketchSolver.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace dc {
namespace sketch {

constexpr float PI = 3.14159265358979323846f;

// ==================== Constraint Implementation ====================

uint64_t Constraint::s_nextId = 1;

Constraint::Constraint(ConstraintType type)
    : m_id(s_nextId++)
    , m_type(type)
{
}

bool Constraint::isSatisfied(const Sketch& sketch, float tolerance) const {
    return std::abs(getError(sketch)) <= tolerance;
}

float Constraint::getError(const Sketch& sketch) const {
    // This is a simplified version - the full implementation would
    // compute the actual constraint error based on type
    return 0.0f;  // Placeholder
}

Constraint::Ptr Constraint::create(ConstraintType type) {
    return std::make_shared<Constraint>(type);
}

// ==================== SketchSolver Implementation ====================

SketchSolver::SketchSolver() = default;

void SketchSolver::setSketch(Sketch::Ptr sketch) {
    m_sketch = std::move(sketch);
}

uint64_t SketchSolver::addConstraint(Constraint::Ptr constraint) {
    if (!constraint) {
        return 0;
    }
    
    uint64_t id = constraint->getId();
    m_constraintIndex[id] = m_constraints.size();
    m_constraints.push_back(std::move(constraint));
    return id;
}

bool SketchSolver::removeConstraint(uint64_t constraintId) {
    auto it = m_constraintIndex.find(constraintId);
    if (it == m_constraintIndex.end()) {
        return false;
    }
    
    size_t index = it->second;
    m_constraints.erase(m_constraints.begin() + index);
    rebuildIndex();
    return true;
}

Constraint::Ptr SketchSolver::getConstraint(uint64_t constraintId) {
    auto it = m_constraintIndex.find(constraintId);
    if (it == m_constraintIndex.end()) {
        return nullptr;
    }
    return m_constraints[it->second];
}

void SketchSolver::clearConstraints() {
    m_constraints.clear();
    m_constraintIndex.clear();
}

int SketchSolver::getDOF() const {
    if (!m_sketch) return 0;
    
    int numVars = getNumVariables();
    int numConstraints = 0;
    
    // Count constraint equations
    for (const auto& constraint : m_constraints) {
        switch (constraint->getType()) {
            case ConstraintType::Coincident:
                numConstraints += 2;  // x and y match
                break;
            case ConstraintType::Horizontal:
            case ConstraintType::Vertical:
            case ConstraintType::Distance:
            case ConstraintType::Angle:
            case ConstraintType::Radius:
            case ConstraintType::Parallel:
            case ConstraintType::Perpendicular:
                numConstraints += 1;
                break;
            case ConstraintType::FixedPoint:
                numConstraints += 2;  // x and y fixed
                break;
            case ConstraintType::Tangent:
                numConstraints += 1;
                break;
            default:
                numConstraints += 1;
        }
    }
    
    return std::max(0, numVars - numConstraints);
}

int SketchSolver::getNumVariables() const {
    if (!m_sketch) return 0;
    
    int numVars = 0;
    for (const auto& entity : m_sketch->getEntities()) {
        switch (entity->getType()) {
            case SketchEntityType::Line:
                numVars += 4;  // 2 points * 2 coords
                break;
            case SketchEntityType::Circle:
                numVars += 3;  // center (2) + radius (1)
                break;
            case SketchEntityType::Arc:
                numVars += 5;  // center (2) + radius (1) + angles (2)
                break;
            case SketchEntityType::Point:
                numVars += 2;
                break;
            case SketchEntityType::Spline: {
                auto spline = std::dynamic_pointer_cast<SketchSpline>(entity);
                if (spline) {
                    numVars += spline->getNumControlPoints() * 2;
                }
                break;
            }
            default:
                break;
        }
    }
    return numVars;
}

std::vector<float> SketchSolver::getVariables() const {
    std::vector<float> vars;
    if (!m_sketch) return vars;
    
    for (const auto& entity : m_sketch->getEntities()) {
        switch (entity->getType()) {
            case SketchEntityType::Line: {
                auto line = std::dynamic_pointer_cast<SketchLine>(entity);
                if (line) {
                    vars.push_back(line->getStart().x);
                    vars.push_back(line->getStart().y);
                    vars.push_back(line->getEnd().x);
                    vars.push_back(line->getEnd().y);
                }
                break;
            }
            case SketchEntityType::Circle: {
                auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
                if (circle) {
                    vars.push_back(circle->getCenter().x);
                    vars.push_back(circle->getCenter().y);
                    vars.push_back(circle->getRadius());
                }
                break;
            }
            case SketchEntityType::Arc: {
                auto arc = std::dynamic_pointer_cast<SketchArc>(entity);
                if (arc) {
                    vars.push_back(arc->getCenter().x);
                    vars.push_back(arc->getCenter().y);
                    vars.push_back(arc->getRadius());
                    vars.push_back(arc->getStartAngle());
                    vars.push_back(arc->getEndAngle());
                }
                break;
            }
            case SketchEntityType::Spline: {
                auto spline = std::dynamic_pointer_cast<SketchSpline>(entity);
                if (spline) {
                    for (int i = 0; i < spline->getNumControlPoints(); ++i) {
                        vars.push_back(spline->getControlPoint(i).x);
                        vars.push_back(spline->getControlPoint(i).y);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return vars;
}

void SketchSolver::setVariables(const std::vector<float>& vars) {
    if (!m_sketch) return;
    
    size_t idx = 0;
    for (auto& entity : m_sketch->getEntities()) {
        switch (entity->getType()) {
            case SketchEntityType::Line: {
                auto line = std::dynamic_pointer_cast<SketchLine>(entity);
                if (line && idx + 4 <= vars.size()) {
                    line->setStart(glm::vec2(vars[idx], vars[idx + 1]));
                    line->setEnd(glm::vec2(vars[idx + 2], vars[idx + 3]));
                    idx += 4;
                }
                break;
            }
            case SketchEntityType::Circle: {
                auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
                if (circle && idx + 3 <= vars.size()) {
                    circle->setCenter(glm::vec2(vars[idx], vars[idx + 1]));
                    circle->setRadius(vars[idx + 2]);
                    idx += 3;
                }
                break;
            }
            case SketchEntityType::Arc: {
                auto arc = std::dynamic_pointer_cast<SketchArc>(entity);
                if (arc && idx + 5 <= vars.size()) {
                    arc->setCenter(glm::vec2(vars[idx], vars[idx + 1]));
                    arc->setRadius(vars[idx + 2]);
                    arc->setStartAngle(vars[idx + 3]);
                    arc->setEndAngle(vars[idx + 4]);
                    idx += 5;
                }
                break;
            }
            case SketchEntityType::Spline: {
                auto spline = std::dynamic_pointer_cast<SketchSpline>(entity);
                if (spline) {
                    for (int i = 0; i < spline->getNumControlPoints() && idx + 2 <= vars.size(); ++i) {
                        spline->setControlPoint(i, glm::vec2(vars[idx], vars[idx + 1]));
                        idx += 2;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

glm::vec2 SketchSolver::getPointFromRef(const ConstraintRef& ref) const {
    if (!m_sketch) return glm::vec2(0.0f);
    
    auto entity = m_sketch->getEntity(ref.entityId);
    if (!entity) return glm::vec2(0.0f);
    
    switch (entity->getType()) {
        case SketchEntityType::Line: {
            auto line = std::dynamic_pointer_cast<SketchLine>(entity);
            if (line) {
                if (ref.pointIndex == 0) return line->getStart();
                if (ref.pointIndex == 1) return line->getEnd();
                return line->midpoint();
            }
            break;
        }
        case SketchEntityType::Circle: {
            auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
            if (circle) {
                return circle->getCenter();
            }
            break;
        }
        case SketchEntityType::Arc: {
            auto arc = std::dynamic_pointer_cast<SketchArc>(entity);
            if (arc) {
                if (ref.pointIndex == 0) return arc->startPoint();
                if (ref.pointIndex == 1) return arc->endPoint();
                if (ref.pointIndex == 2) return arc->getCenter();
                return arc->midPoint();
            }
            break;
        }
        case SketchEntityType::Spline: {
            auto spline = std::dynamic_pointer_cast<SketchSpline>(entity);
            if (spline) {
                if (ref.pointIndex == 0) return spline->startPoint();
                if (ref.pointIndex == 1) return spline->endPoint();
            }
            break;
        }
        default:
            break;
    }
    
    return glm::vec2(0.0f);
}

int SketchSolver::buildEquations(std::vector<float>& residuals) {
    residuals.clear();
    
    for (const auto& constraint : m_constraints) {
        int eqIndex = static_cast<int>(residuals.size());
        evaluateConstraint(*constraint, residuals, eqIndex);
    }
    
    return static_cast<int>(residuals.size());
}

void SketchSolver::evaluateConstraint(const Constraint& constraint, 
                                       std::vector<float>& residuals, int& eqIndex) {
    switch (constraint.getType()) {
        case ConstraintType::Coincident: {
            if (constraint.refs.size() >= 2) {
                glm::vec2 p1 = getPointFromRef(constraint.refs[0]);
                glm::vec2 p2 = getPointFromRef(constraint.refs[1]);
                residuals.push_back(p1.x - p2.x);
                residuals.push_back(p1.y - p2.y);
            }
            break;
        }
        
        case ConstraintType::Horizontal: {
            if (constraint.refs.size() >= 1) {
                auto entity = m_sketch->getEntity(constraint.refs[0].entityId);
                if (auto line = std::dynamic_pointer_cast<SketchLine>(entity)) {
                    residuals.push_back(line->getEnd().y - line->getStart().y);
                }
            }
            break;
        }
        
        case ConstraintType::Vertical: {
            if (constraint.refs.size() >= 1) {
                auto entity = m_sketch->getEntity(constraint.refs[0].entityId);
                if (auto line = std::dynamic_pointer_cast<SketchLine>(entity)) {
                    residuals.push_back(line->getEnd().x - line->getStart().x);
                }
            }
            break;
        }
        
        case ConstraintType::Distance: {
            if (constraint.refs.size() >= 2) {
                glm::vec2 p1 = getPointFromRef(constraint.refs[0]);
                glm::vec2 p2 = getPointFromRef(constraint.refs[1]);
                float dist = glm::length(p2 - p1);
                residuals.push_back(dist - constraint.value);
            }
            break;
        }
        
        case ConstraintType::Parallel: {
            if (constraint.refs.size() >= 2) {
                auto e1 = m_sketch->getEntity(constraint.refs[0].entityId);
                auto e2 = m_sketch->getEntity(constraint.refs[1].entityId);
                auto line1 = std::dynamic_pointer_cast<SketchLine>(e1);
                auto line2 = std::dynamic_pointer_cast<SketchLine>(e2);
                if (line1 && line2) {
                    glm::vec2 d1 = line1->direction();
                    glm::vec2 d2 = line2->direction();
                    // Cross product should be zero for parallel
                    residuals.push_back(d1.x * d2.y - d1.y * d2.x);
                }
            }
            break;
        }
        
        case ConstraintType::Perpendicular: {
            if (constraint.refs.size() >= 2) {
                auto e1 = m_sketch->getEntity(constraint.refs[0].entityId);
                auto e2 = m_sketch->getEntity(constraint.refs[1].entityId);
                auto line1 = std::dynamic_pointer_cast<SketchLine>(e1);
                auto line2 = std::dynamic_pointer_cast<SketchLine>(e2);
                if (line1 && line2) {
                    glm::vec2 d1 = line1->direction();
                    glm::vec2 d2 = line2->direction();
                    // Dot product should be zero for perpendicular
                    residuals.push_back(d1.x * d2.x + d1.y * d2.y);
                }
            }
            break;
        }
        
        case ConstraintType::Angle: {
            if (constraint.refs.size() >= 2) {
                auto e1 = m_sketch->getEntity(constraint.refs[0].entityId);
                auto e2 = m_sketch->getEntity(constraint.refs[1].entityId);
                auto line1 = std::dynamic_pointer_cast<SketchLine>(e1);
                auto line2 = std::dynamic_pointer_cast<SketchLine>(e2);
                if (line1 && line2) {
                    float a1 = line1->angle();
                    float a2 = line2->angle();
                    float diff = a2 - a1;
                    // Normalize to [-π, π]
                    while (diff > PI) diff -= 2 * PI;
                    while (diff < -PI) diff += 2 * PI;
                    residuals.push_back(diff - constraint.value);
                }
            }
            break;
        }
        
        case ConstraintType::Radius: {
            if (constraint.refs.size() >= 1) {
                auto entity = m_sketch->getEntity(constraint.refs[0].entityId);
                if (auto circle = std::dynamic_pointer_cast<SketchCircle>(entity)) {
                    residuals.push_back(circle->getRadius() - constraint.value);
                } else if (auto arc = std::dynamic_pointer_cast<SketchArc>(entity)) {
                    residuals.push_back(arc->getRadius() - constraint.value);
                }
            }
            break;
        }
        
        case ConstraintType::FixedPoint: {
            if (constraint.refs.size() >= 1) {
                glm::vec2 p = getPointFromRef(constraint.refs[0]);
                // Value encodes the fixed position (we'd need a better way to store this)
                // For now, use refs[1] if available, or (value, 0)
                glm::vec2 target(constraint.value, 0.0f);
                if (constraint.refs.size() >= 2) {
                    target = glm::vec2(constraint.refs[1].entityId, constraint.refs[1].pointIndex);
                }
                residuals.push_back(p.x - target.x);
                residuals.push_back(p.y - target.y);
            }
            break;
        }
        
        case ConstraintType::Tangent: {
            // Tangent constraint is more complex - simplified version
            if (constraint.refs.size() >= 2) {
                // For line-circle tangent
                auto e1 = m_sketch->getEntity(constraint.refs[0].entityId);
                auto e2 = m_sketch->getEntity(constraint.refs[1].entityId);
                
                auto line = std::dynamic_pointer_cast<SketchLine>(e1);
                auto circle = std::dynamic_pointer_cast<SketchCircle>(e2);
                
                if (line && circle) {
                    // Distance from center to line should equal radius
                    glm::vec2 lineDir = line->direction();
                    glm::vec2 toCenter = circle->getCenter() - line->getStart();
                    float dist = std::abs(toCenter.x * lineDir.y - toCenter.y * lineDir.x);
                    residuals.push_back(dist - circle->getRadius());
                }
            }
            break;
        }
        
        default:
            break;
    }
}

void SketchSolver::buildJacobian(std::vector<std::vector<float>>& jacobian, 
                                  int numEqs, int numVars) {
    const float h = 1e-7f;  // Finite difference step
    
    jacobian.resize(numEqs);
    for (auto& row : jacobian) {
        row.resize(numVars, 0.0f);
    }
    
    std::vector<float> vars = getVariables();
    std::vector<float> residuals0;
    buildEquations(residuals0);
    
    // Compute Jacobian using finite differences
    for (int j = 0; j < numVars; ++j) {
        std::vector<float> varsPert = vars;
        varsPert[j] += h;
        setVariables(varsPert);
        
        std::vector<float> residualsPert;
        buildEquations(residualsPert);
        
        for (int i = 0; i < numEqs && i < static_cast<int>(residuals0.size()) 
             && i < static_cast<int>(residualsPert.size()); ++i) {
            jacobian[i][j] = (residualsPert[i] - residuals0[i]) / h;
        }
    }
    
    // Restore original variables
    setVariables(vars);
}

bool SketchSolver::solveLinearSystem(const std::vector<std::vector<float>>& J,
                                      const std::vector<float>& residuals,
                                      std::vector<float>& dx) {
    int m = static_cast<int>(J.size());      // Number of equations
    int n = m > 0 ? static_cast<int>(J[0].size()) : 0;  // Number of variables
    
    if (m == 0 || n == 0) return false;
    
    dx.resize(n, 0.0f);
    
    // Use least squares if overdetermined, or solve directly if square
    // This is a simplified Gauss-Seidel-like approach
    
    // Build J^T * J and J^T * r for normal equations
    std::vector<std::vector<float>> JTJ(n, std::vector<float>(n, 0.0f));
    std::vector<float> JTr(n, 0.0f);
    
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < m; ++k) {
                JTJ[i][j] += J[k][i] * J[k][j];
            }
        }
        for (int k = 0; k < m; ++k) {
            JTr[i] += J[k][i] * residuals[k];
        }
    }
    
    // Add small regularization for numerical stability
    for (int i = 0; i < n; ++i) {
        JTJ[i][i] += 1e-8f;
    }
    
    // Solve using Gaussian elimination with partial pivoting
    std::vector<std::vector<float>> A = JTJ;
    std::vector<float> b = JTr;
    
    for (int i = 0; i < n; ++i) {
        // Find pivot
        int maxRow = i;
        for (int k = i + 1; k < n; ++k) {
            if (std::abs(A[k][i]) > std::abs(A[maxRow][i])) {
                maxRow = k;
            }
        }
        std::swap(A[i], A[maxRow]);
        std::swap(b[i], b[maxRow]);
        
        if (std::abs(A[i][i]) < 1e-12f) {
            continue;  // Skip singular row
        }
        
        // Eliminate column
        for (int k = i + 1; k < n; ++k) {
            float factor = A[k][i] / A[i][i];
            for (int j = i; j < n; ++j) {
                A[k][j] -= factor * A[i][j];
            }
            b[k] -= factor * b[i];
        }
    }
    
    // Back substitution
    for (int i = n - 1; i >= 0; --i) {
        if (std::abs(A[i][i]) < 1e-12f) {
            dx[i] = 0.0f;
            continue;
        }
        dx[i] = b[i];
        for (int j = i + 1; j < n; ++j) {
            dx[i] -= A[i][j] * dx[j];
        }
        dx[i] /= A[i][i];
    }
    
    return true;
}

SolveResult SketchSolver::solve() {
    SolveResult result;
    
    if (!m_sketch) {
        result.status = SolverStatus::InvalidInput;
        return result;
    }
    
    int numVars = getNumVariables();
    if (numVars == 0) {
        result.status = SolverStatus::Success;
        result.degreesOfFreedom = 0;
        return result;
    }
    
    std::vector<float> vars = getVariables();
    
    for (int iter = 0; iter < m_maxIterations; ++iter) {
        result.iterations = iter + 1;
        
        // Build residuals
        std::vector<float> residuals;
        int numEqs = buildEquations(residuals);
        
        if (numEqs == 0) {
            result.status = SolverStatus::UnderConstrained;
            result.degreesOfFreedom = numVars;
            return result;
        }
        
        // Check convergence
        float error = 0.0f;
        for (float r : residuals) {
            error += r * r;
        }
        error = std::sqrt(error);
        result.finalError = error;
        
        if (error < m_tolerance) {
            result.status = numEqs < numVars ? SolverStatus::UnderConstrained : SolverStatus::Success;
            result.degreesOfFreedom = getDOF();
            return result;
        }
        
        // Build Jacobian
        std::vector<std::vector<float>> jacobian;
        buildJacobian(jacobian, numEqs, numVars);
        
        // Solve for update
        std::vector<float> dx;
        if (!solveLinearSystem(jacobian, residuals, dx)) {
            result.status = SolverStatus::NotConverged;
            return result;
        }
        
        // Apply update with line search
        float alpha = 1.0f;
        for (int lsIter = 0; lsIter < 10; ++lsIter) {
            std::vector<float> newVars = vars;
            for (size_t i = 0; i < vars.size() && i < dx.size(); ++i) {
                newVars[i] = vars[i] - alpha * dx[i];
            }
            setVariables(newVars);
            
            std::vector<float> newResiduals;
            buildEquations(newResiduals);
            
            float newError = 0.0f;
            for (float r : newResiduals) {
                newError += r * r;
            }
            newError = std::sqrt(newError);
            
            if (newError < error) {
                vars = newVars;
                break;
            }
            
            alpha *= 0.5f;
        }
    }
    
    result.status = SolverStatus::NotConverged;
    result.degreesOfFreedom = getDOF();
    return result;
}

// ==================== Convenience Methods ====================

uint64_t SketchSolver::addCoincident(uint64_t entity1, int point1, uint64_t entity2, int point2) {
    auto c = Constraint::create(ConstraintType::Coincident);
    c->refs.push_back({entity1, point1});
    c->refs.push_back({entity2, point2});
    return addConstraint(c);
}

uint64_t SketchSolver::addHorizontal(uint64_t lineId) {
    auto c = Constraint::create(ConstraintType::Horizontal);
    c->refs.push_back({lineId, -1});
    return addConstraint(c);
}

uint64_t SketchSolver::addVertical(uint64_t lineId) {
    auto c = Constraint::create(ConstraintType::Vertical);
    c->refs.push_back({lineId, -1});
    return addConstraint(c);
}

uint64_t SketchSolver::addParallel(uint64_t line1Id, uint64_t line2Id) {
    auto c = Constraint::create(ConstraintType::Parallel);
    c->refs.push_back({line1Id, -1});
    c->refs.push_back({line2Id, -1});
    return addConstraint(c);
}

uint64_t SketchSolver::addPerpendicular(uint64_t line1Id, uint64_t line2Id) {
    auto c = Constraint::create(ConstraintType::Perpendicular);
    c->refs.push_back({line1Id, -1});
    c->refs.push_back({line2Id, -1});
    return addConstraint(c);
}

uint64_t SketchSolver::addDistance(uint64_t entity1, int point1, uint64_t entity2, int point2, float distance) {
    auto c = Constraint::create(ConstraintType::Distance);
    c->refs.push_back({entity1, point1});
    c->refs.push_back({entity2, point2});
    c->value = distance;
    return addConstraint(c);
}

uint64_t SketchSolver::addAngle(uint64_t line1Id, uint64_t line2Id, float angleRadians) {
    auto c = Constraint::create(ConstraintType::Angle);
    c->refs.push_back({line1Id, -1});
    c->refs.push_back({line2Id, -1});
    c->value = angleRadians;
    return addConstraint(c);
}

uint64_t SketchSolver::addRadius(uint64_t entityId, float radius) {
    auto c = Constraint::create(ConstraintType::Radius);
    c->refs.push_back({entityId, -1});
    c->value = radius;
    return addConstraint(c);
}

uint64_t SketchSolver::addTangent(uint64_t entity1Id, uint64_t entity2Id) {
    auto c = Constraint::create(ConstraintType::Tangent);
    c->refs.push_back({entity1Id, -1});
    c->refs.push_back({entity2Id, -1});
    return addConstraint(c);
}

uint64_t SketchSolver::addFixedPoint(uint64_t entityId, int pointIndex, const glm::vec2& position) {
    auto c = Constraint::create(ConstraintType::FixedPoint);
    c->refs.push_back({entityId, pointIndex});
    // Store position in a hacky way - ideally we'd have a better structure
    c->refs.push_back({static_cast<uint64_t>(position.x * 1000), static_cast<int>(position.y * 1000)});
    return addConstraint(c);
}

void SketchSolver::rebuildIndex() {
    m_constraintIndex.clear();
    for (size_t i = 0; i < m_constraints.size(); ++i) {
        m_constraintIndex[m_constraints[i]->getId()] = i;
    }
}

SketchSolver::Ptr SketchSolver::create() {
    return std::make_shared<SketchSolver>();
}

} // namespace sketch
} // namespace dc
