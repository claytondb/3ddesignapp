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
 * IGES entity type codes
 */
enum class IGESEntityType {
    CIRCULAR_ARC = 100,
    COMPOSITE_CURVE = 102,
    CONIC_ARC = 104,
    COPIOUS_DATA = 106,
    PLANE = 108,
    LINE = 110,
    PARAMETRIC_SPLINE_CURVE = 112,
    PARAMETRIC_SPLINE_SURFACE = 114,
    POINT = 116,
    RULED_SURFACE = 118,
    SURFACE_OF_REVOLUTION = 120,
    TABULATED_CYLINDER = 122,
    TRANSFORMATION_MATRIX = 124,
    RATIONAL_B_SPLINE_CURVE = 126,
    RATIONAL_B_SPLINE_SURFACE = 128,
    OFFSET_CURVE = 130,
    CONNECT_POINT = 132,
    NODE = 134,
    FINITE_ELEMENT = 136,
    NODAL_DISPLACEMENT = 138,
    OFFSET_SURFACE = 140,
    TRIMMED_PARAMETRIC_SURFACE = 144,
    NODAL_RESULTS = 146,
    ELEMENT_RESULTS = 148,
    BLOCK = 150,
    RIGHT_ANGULAR_WEDGE = 152,
    RIGHT_CIRCULAR_CYLINDER = 154,
    RIGHT_CIRCULAR_CONE_FRUSTUM = 156,
    SPHERE = 158,
    TORUS = 160,
    SOLID_OF_REVOLUTION = 162,
    SOLID_OF_LINEAR_EXTRUSION = 164,
    ELLIPSOID = 168,
    BOOLEAN_TREE = 180,
    SELECTED_COMPONENT = 182,
    SOLID_ASSEMBLY = 184,
    MANIFOLD_SOLID_B_REP = 186,
    PLANE_SURFACE = 190,
    RIGHT_CIRCULAR_CYLINDRICAL_SURFACE = 192,
    RIGHT_CIRCULAR_CONICAL_SURFACE = 194,
    SPHERICAL_SURFACE = 196,
    TOROIDAL_SURFACE = 198,
    DIRECTION = 123,
    COLOR_DEFINITION = 314,
    UNITS_DATA = 316,
    NETWORK_SUBFIGURE_DEFINITION = 320,
    ATTRIBUTE_TABLE_DEFINITION = 322,
    ASSOCIATIVITY_INSTANCE = 402,
    DRAWING = 404,
    PROPERTY = 406,
    SINGULAR_SUBFIGURE_INSTANCE = 408,
    VIEW = 410,
    RECTANGULAR_ARRAY_INSTANCE = 412,
    CIRCULAR_ARRAY_INSTANCE = 414,
    EXTERNAL_REFERENCE = 416,
    NODAL_LOAD = 418,
    NETWORK_SUBFIGURE_INSTANCE = 420,
    ATTRIBUTE_TABLE_INSTANCE = 422,
    LABEL_DISPLAY = 402
};

/**
 * IGES entity data structure
 */
struct IGESEntity {
    int directoryEntryLine;     // Line number in directory section
    int parameterDataLine;      // Line number in parameter section
    IGESEntityType type;
    int structure = 0;
    int lineFontPattern = 0;
    int level = 0;
    int view = 0;
    int transformationMatrix = 0;
    int labelDisplayAssoc = 0;
    int statusNumber = 0;
    int lineWeight = 0;
    int colorNumber = 0;
    int parameterLineCount = 0;
    int formNumber = 0;
    std::string entityLabel;
    std::string entitySubscript;
    std::string parameterData;
    
    IGESEntity(IGESEntityType type) : type(type), directoryEntryLine(0), parameterDataLine(0) {}
};

/**
 * IGES file exporter
 * Exports geometry in Initial Graphics Exchange Specification format
 */
class IGESExporter {
public:
    IGESExporter();
    ~IGESExporter();
    
    /**
     * Export a model to IGES format
     * @param model The model to export
     * @param filename Output file path
     * @param options Export options
     * @return true if export successful
     */
    bool exportModel(const Model& model, const std::string& filename, const ExportOptions& options);
    
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
    std::vector<IGESEntity> m_entities;
    std::map<void*, int> m_entityMap;
    std::string m_errorMessage;
    ExportOptions m_options;
    
    // File sections
    std::string m_startSection;
    std::string m_globalSection;
    std::vector<std::string> m_directorySection;
    std::vector<std::string> m_parameterSection;
    
    // Line counters
    int m_directoryLineCount;
    int m_parameterLineCount;
    
    // Section building
    void buildStartSection(const std::string& filename);
    void buildGlobalSection(const std::string& filename);
    void buildDirectorySection();
    void buildParameterSection();
    
    // File output
    bool writeFile(const std::string& filename);
    std::string formatLine(const std::string& content, char section, int lineNumber);
    std::string padRight(const std::string& str, int width);
    std::string padLeft(const std::string& str, int width);
    
    // Entity creation
    int addEntity(IGESEntity& entity);
    
    // Geometry export
    int exportPoint(const glm::dvec3& point);
    int exportLine(const glm::dvec3& start, const glm::dvec3& end);
    int exportCircularArc(const glm::dvec3& center, const glm::dvec3& start, const glm::dvec3& end);
    int exportNURBSCurve(const NURBSCurve& curve);
    int exportNURBSSurface(const NURBSSurface& surface);
    int exportTrimmedSurface(const Face& face, int surfaceId, const std::vector<int>& curveIds);
    int exportTransformationMatrix(const glm::dmat4& matrix);
    int exportColorDefinition(const glm::vec3& color);
    
    // Body/face export
    int exportFace(const Face& face);
    int exportBody(const Body& body);
    
    // Parameter data formatting
    std::string formatParameterData(IGESEntityType type, const std::vector<std::string>& params);
    std::string formatReal(double value) const;
    std::string formatInt(int value) const;
    
    // Coordinate transformation
    glm::dvec3 transformPoint(const glm::dvec3& point) const;
    glm::dvec3 transformDirection(const glm::dvec3& dir) const;
};

} // namespace dc
