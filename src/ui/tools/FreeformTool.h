#pragma once

#include "Tool.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace dc {

class QuadMesh;
class NurbsSurface;
class SceneObject;

/**
 * @brief Editing mode for freeform tool
 */
enum class FreeformEditMode {
    Select,             // Select control points/faces
    Move,               // Move selected elements
    Smooth,             // Smooth selected vertices
    Crease,             // Mark/unmark crease edges
    Subdivide,          // Subdivide selection
    AddControlPoint,    // Add new control point
    RemoveControlPoint, // Remove control point
    ExtrudeFace,        // Extrude selected face
    InsertEdgeLoop,     // Insert edge loop
    Sculpt              // Sculpt mode (push/pull)
};

/**
 * @brief Selection mode for freeform tool
 */
enum class FreeformSelectMode {
    Vertex,
    Edge,
    Face,
    EdgeLoop,
    FaceLoop
};

/**
 * @brief Sculpt brush settings
 */
struct SculptBrushSettings {
    float radius = 0.1f;
    float strength = 0.5f;
    float falloff = 0.5f;  // 0 = sharp, 1 = smooth falloff
    bool symmetry = false;
    glm::vec3 symmetryAxis = glm::vec3(1, 0, 0);
};

/**
 * @brief Tool for freeform/organic surface modeling
 * 
 * Provides interactive control point manipulation, subdivision control,
 * and sculpting capabilities for quad meshes and subdivision surfaces.
 */
class FreeformTool : public Tool {
public:
    FreeformTool();
    ~FreeformTool() override;
    
    // Tool interface
    std::string getName() const override { return "Freeform"; }
    std::string getDescription() const override { 
        return "Edit freeform surfaces and subdivision meshes"; 
    }
    std::string getIcon() const override { return "freeform"; }
    
    void activate() override;
    void deactivate() override;
    
    // Input handling
    bool onMousePress(const MouseEvent& event) override;
    bool onMouseRelease(const MouseEvent& event) override;
    bool onMouseMove(const MouseEvent& event) override;
    bool onMouseWheel(const WheelEvent& event) override;
    bool onKeyPress(const KeyEvent& event) override;
    bool onKeyRelease(const KeyEvent& event) override;
    
    // Rendering
    void render(Renderer& renderer) override;
    void renderOverlay(Renderer& renderer) override;
    
    // Edit mode
    void setEditMode(FreeformEditMode mode);
    FreeformEditMode getEditMode() const { return m_editMode; }
    
    // Selection mode
    void setSelectMode(FreeformSelectMode mode);
    FreeformSelectMode getSelectMode() const { return m_selectMode; }
    
    // Subdivision
    void setSubdivisionLevel(int level);
    int getSubdivisionLevel() const { return m_subdivisionLevel; }
    void setShowControlMesh(bool show) { m_showControlMesh = show; }
    bool getShowControlMesh() const { return m_showControlMesh; }
    
    // Crease operations
    void setCreaseWeight(float weight) { m_creaseWeight = weight; }
    float getCreaseWeight() const { return m_creaseWeight; }
    void markCreaseEdges();
    void unmarkCreaseEdges();
    
    // Sculpt brush
    void setBrushSettings(const SculptBrushSettings& settings);
    const SculptBrushSettings& getBrushSettings() const { return m_brushSettings; }
    
    // Actions
    void smoothSelection(float factor = 0.5f);
    void relaxSelection(int iterations = 1);
    void flattenSelection();
    void subdivideSelection();
    void extrudeSelection(float distance);
    void insertEdgeLoop();
    void deleteSelection();
    
    // Conversion
    void convertToNurbs();
    void convertToQuadMesh();
    
    // Selection
    void selectAll();
    void selectNone();
    void selectConnected();
    void invertSelection();
    void growSelection();
    void shrinkSelection();
    
    // Target mesh
    void setTargetObject(std::shared_ptr<SceneObject> object);
    std::shared_ptr<SceneObject> getTargetObject() const { return m_targetObject; }
    
private:
    // Edit state
    FreeformEditMode m_editMode = FreeformEditMode::Select;
    FreeformSelectMode m_selectMode = FreeformSelectMode::Vertex;
    
    // Target
    std::shared_ptr<SceneObject> m_targetObject;
    std::shared_ptr<QuadMesh> m_quadMesh;
    std::shared_ptr<QuadMesh> m_subdividedMesh; // Preview mesh
    
    // Subdivision
    int m_subdivisionLevel = 2;
    bool m_showControlMesh = true;
    bool m_subdividedDirty = true;
    
    // Crease
    float m_creaseWeight = 1.0f;
    
    // Sculpt
    SculptBrushSettings m_brushSettings;
    bool m_isSculpting = false;
    
    // Interaction state
    bool m_isDragging = false;
    glm::vec2 m_dragStart;
    glm::vec2 m_lastMousePos;
    glm::vec3 m_dragPlaneNormal;
    glm::vec3 m_dragPlaneOrigin;
    std::vector<glm::vec3> m_dragStartPositions;
    
    // Selection
    std::vector<int> m_selectedVertices;
    std::vector<int> m_selectedEdges;
    std::vector<int> m_selectedFaces;
    int m_hoveredVertex = -1;
    int m_hoveredEdge = -1;
    int m_hoveredFace = -1;
    
    // Box selection
    bool m_isBoxSelecting = false;
    glm::vec2 m_boxSelectStart;
    glm::vec2 m_boxSelectEnd;
    
    // Helpers
    void updateSubdividedMesh();
    void pickElement(const glm::vec2& screenPos, bool addToSelection);
    void boxSelect(const glm::vec2& min, const glm::vec2& max, bool addToSelection);
    
    int pickVertex(const glm::vec2& screenPos) const;
    int pickEdge(const glm::vec2& screenPos) const;
    int pickFace(const glm::vec2& screenPos) const;
    
    void beginDrag(const glm::vec2& screenPos);
    void updateDrag(const glm::vec2& screenPos);
    void endDrag();
    
    void applySculptBrush(const glm::vec3& center, const glm::vec3& direction);
    float computeBrushFalloff(float distance) const;
    
    glm::vec3 screenToWorld(const glm::vec2& screenPos, float depth) const;
    glm::vec3 projectOntoPlane(const glm::vec2& screenPos,
                                const glm::vec3& planeOrigin,
                                const glm::vec3& planeNormal) const;
    
    // Rendering helpers
    void renderControlPoints(Renderer& renderer);
    void renderControlMesh(Renderer& renderer);
    void renderCreaseEdges(Renderer& renderer);
    void renderSelection(Renderer& renderer);
    void renderBrushCursor(Renderer& renderer);
};

/**
 * @brief Property panel for FreeformTool
 */
class FreeformToolPanel : public ToolPanel {
public:
    FreeformToolPanel(FreeformTool* tool);
    
    void render() override;
    
private:
    FreeformTool* m_tool;
    
    void renderModeSelector();
    void renderSubdivisionControls();
    void renderCreaseControls();
    void renderSculptControls();
    void renderSelectionActions();
    void renderConversionActions();
};

} // namespace dc
