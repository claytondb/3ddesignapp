#pragma once

#include "ExportOptions.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <map>
#include <glm/glm.hpp>

namespace dc {

// Forward declarations
class Model;
class Body;
class Face;
class Edge;
class NURBSSurface;
class NURBSCurve;

/**
 * STEP entity types for export
 */
enum class STEPEntityType {
    CARTESIAN_POINT,
    DIRECTION,
    VECTOR,
    AXIS2_PLACEMENT_3D,
    LINE,
    CIRCLE,
    ELLIPSE,
    B_SPLINE_CURVE_WITH_KNOTS,
    B_SPLINE_SURFACE_WITH_KNOTS,
    PLANE,
    CYLINDRICAL_SURFACE,
    CONICAL_SURFACE,
    SPHERICAL_SURFACE,
    TOROIDAL_SURFACE,
    VERTEX_POINT,
    EDGE_CURVE,
    EDGE_LOOP,
    FACE_OUTER_BOUND,
    FACE_BOUND,
    ADVANCED_FACE,
    CLOSED_SHELL,
    OPEN_SHELL,
    MANIFOLD_SOLID_BREP,
    SHELL_BASED_SURFACE_MODEL,
    GEOMETRIC_CURVE_SET,
    SHAPE_REPRESENTATION,
    PRODUCT,
    PRODUCT_DEFINITION,
    PRODUCT_DEFINITION_FORMATION,
    PRODUCT_DEFINITION_SHAPE,
    SHAPE_DEFINITION_REPRESENTATION,
    COLOUR_RGB,
    SURFACE_STYLE_USAGE,
    PRESENTED_ITEM_REPRESENTATION,
    STYLED_ITEM,
    MECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION
};

/**
 * STEP entity reference
 */
struct STEPEntity {
    int id;
    STEPEntityType type;
    std::string data;
    
    STEPEntity(int id, STEPEntityType type, const std::string& data)
        : id(id), type(type), data(data) {}
};

/**
 * STEP file exporter
 * Supports AP203 (geometry only) and AP214 (with colors/presentation)
 */
class STEPExporter {
public:
    STEPExporter();
    ~STEPExporter();
    
    /**
     * Export a model to STEP format
     * @param model The model to export
     * @param filename Output file path
     * @param options Export options
     * @return true if export successful
     */
    bool exportModel(const Model& model, const std::string& filename, const ExportOptions& options);
    
    /**
     * Export multiple bodies as an assembly
     */
    bool exportAssembly(const std::vector<std::shared_ptr<Body>>& bodies,
                        const std::string& filename,
                        const ExportOptions& options);
    
    /**
     * Get last error message
     */
    const std::string& getErrorMessage() const { return m_errorMessage; }
    
    /**
     * Estimate export file size
     */
    size_t estimateFileSize(const Model& model, const ExportOptions& options) const;
    
private:
    // Entity management
    int m_nextEntityId;
    std::vector<STEPEntity> m_entities;
    std::map<void*, int> m_entityMap;  // Map objects to entity IDs
    std::string m_errorMessage;
    ExportOptions m_options;
    
    // File output
    std::ofstream m_file;
    
    // Header writing
    void writeHeader(const std::string& filename);
    void writeDataSection();
    void writeFooter();
    
    // Entity creation helpers
    int addEntity(STEPEntityType type, const std::string& data);
    int getOrCreateEntity(void* object, STEPEntityType type, const std::string& data);
    
    // Geometry export
    int exportCartesianPoint(const glm::dvec3& point);
    int exportDirection(const glm::dvec3& dir);
    int exportVector(const glm::dvec3& dir, double magnitude);
    int exportAxis2Placement3D(const glm::dvec3& origin, const glm::dvec3& zDir, const glm::dvec3& xDir);
    
    // Curve export
    int exportLine(const glm::dvec3& start, const glm::dvec3& dir);
    int exportCircle(const glm::dvec3& center, const glm::dvec3& normal, double radius);
    int exportBSplineCurve(const NURBSCurve& curve);
    
    // Surface export
    int exportPlane(const glm::dvec3& origin, const glm::dvec3& normal);
    int exportCylindricalSurface(const glm::dvec3& origin, const glm::dvec3& axis, double radius);
    int exportBSplineSurface(const NURBSSurface& surface);
    
    // Topology export
    int exportVertexPoint(const glm::dvec3& point);
    int exportEdgeCurve(const Edge& edge);
    int exportEdgeLoop(const std::vector<int>& edgeIds);
    int exportFace(const Face& face);
    int exportShell(const std::vector<int>& faceIds, bool closed);
    int exportSolidBrep(const Body& body);
    
    // Product structure (AP214)
    int exportProduct(const std::string& name, const std::string& description);
    int exportProductDefinition(int productId);
    int exportShapeRepresentation(const std::vector<int>& itemIds);
    
    // Color/style export (AP214)
    int exportColorRGB(const glm::vec3& color);
    int exportSurfaceStyle(int colorId);
    int exportStyledItem(int itemId, int styleId);
    
    // Utility functions
    std::string formatReal(double value) const;
    std::string formatPoint(const glm::dvec3& p) const;
    std::string formatDirection(const glm::dvec3& d) const;
    std::string escapeString(const std::string& str) const;
    std::string getEntityTypeName(STEPEntityType type) const;
    std::string getCurrentTimestamp() const;
    
    glm::dvec3 transformPoint(const glm::dvec3& point) const;
    glm::dvec3 transformDirection(const glm::dvec3& dir) const;
};

} // namespace dc
