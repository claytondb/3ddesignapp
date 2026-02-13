#pragma once

#include "ExportOptions.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>
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
 * Parsed STEP entity
 */
struct ParsedSTEPEntity {
    int id;
    std::string typeName;
    std::string rawData;
    std::vector<std::string> parameters;
    bool parsed = false;
};

/**
 * STEP geometry types
 */
struct STEPCartesianPoint {
    glm::dvec3 coords;
};

struct STEPDirection {
    glm::dvec3 ratios;
};

struct STEPAxis2Placement3D {
    int locationId;
    int axisId;
    int refDirectionId;
};

struct STEPBSplineCurve {
    int degree;
    std::vector<int> controlPointIds;
    std::vector<int> knotMultiplicities;
    std::vector<double> knots;
    std::vector<double> weights;
    bool isRational;
};

struct STEPBSplineSurface {
    int degreeU, degreeV;
    std::vector<std::vector<int>> controlPointIds;
    std::vector<int> knotMultsU, knotMultsV;
    std::vector<double> knotsU, knotsV;
    std::vector<std::vector<double>> weights;
    bool isRational;
};

/**
 * STEP file importer
 * Imports STEP AP203 and AP214 files
 */
class STEPImporter {
public:
    STEPImporter();
    ~STEPImporter();
    
    /**
     * Import a STEP file
     * @param filename Input file path
     * @param options Import options
     * @return Imported model, or nullptr on failure
     */
    std::shared_ptr<Model> importFile(const std::string& filename, const ImportOptions& options);
    
    /**
     * Get last error message
     */
    const std::string& getErrorMessage() const { return m_errorMessage; }
    
    /**
     * Get import statistics
     */
    struct ImportStats {
        int totalEntities = 0;
        int facesImported = 0;
        int curvesImported = 0;
        int surfacesImported = 0;
        int bodiesImported = 0;
        std::vector<std::string> warnings;
    };
    
    const ImportStats& getStats() const { return m_stats; }
    
private:
    std::map<int, ParsedSTEPEntity> m_entities;
    std::map<int, glm::dvec3> m_points;
    std::map<int, glm::dvec3> m_directions;
    std::map<int, std::shared_ptr<NURBSCurve>> m_curves;
    std::map<int, std::shared_ptr<NURBSSurface>> m_surfaces;
    std::map<int, std::shared_ptr<Face>> m_faces;
    std::map<int, std::shared_ptr<Body>> m_bodies;
    std::map<int, glm::vec3> m_colors;
    
    std::string m_errorMessage;
    ImportStats m_stats;
    ImportOptions m_options;
    
    // Parsing
    bool parseFile(const std::string& filename);
    bool parseHeader(std::istream& stream);
    bool parseDataSection(std::istream& stream);
    ParsedSTEPEntity parseEntity(const std::string& line);
    std::vector<std::string> parseParameters(const std::string& data);
    
    // Entity processing
    bool processEntities();
    
    // Geometry extraction
    glm::dvec3 getCartesianPoint(int entityId);
    glm::dvec3 getDirection(int entityId);
    glm::dvec3 getVector(int entityId);
    glm::dmat4 getAxis2Placement3D(int entityId);
    
    // Curve extraction
    std::shared_ptr<NURBSCurve> getLine(int entityId);
    std::shared_ptr<NURBSCurve> getCircle(int entityId);
    std::shared_ptr<NURBSCurve> getBSplineCurve(int entityId);
    std::shared_ptr<NURBSCurve> getEdgeCurve(int entityId);
    
    // Surface extraction
    std::shared_ptr<NURBSSurface> getPlane(int entityId);
    std::shared_ptr<NURBSSurface> getCylindricalSurface(int entityId);
    std::shared_ptr<NURBSSurface> getSphericalSurface(int entityId);
    std::shared_ptr<NURBSSurface> getBSplineSurface(int entityId);
    
    // Topology extraction
    std::shared_ptr<Edge> getEdgeCurveAsEdge(int entityId);
    std::vector<std::shared_ptr<Edge>> getEdgeLoop(int entityId);
    std::shared_ptr<Face> getAdvancedFace(int entityId);
    std::shared_ptr<Body> getManifoldSolidBrep(int entityId);
    std::shared_ptr<Body> getShellBasedModel(int entityId);
    std::vector<int> getShellFaceIds(int shellId);
    
    // Color extraction
    glm::vec3 getColorRGB(int entityId);
    void processStyledItems();
    
    // Product structure
    std::string getProductName(int productId);
    
    // Utility functions
    int parseEntityRef(const std::string& ref);
    double parseReal(const std::string& str);
    int parseInt(const std::string& str);
    std::string parseString(const std::string& str);
    bool parseBool(const std::string& str);
    std::vector<int> parseIntList(const std::string& str);
    std::vector<double> parseRealList(const std::string& str);
    std::vector<int> parseEntityRefList(const std::string& str);
    
    void addWarning(const std::string& msg);
};

} // namespace dc
