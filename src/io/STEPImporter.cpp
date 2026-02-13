#include "STEPImporter.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace dc {

// Placeholder classes - same as exporter
class NURBSSurface {
public:
    int degreeU = 3, degreeV = 3;
    std::vector<std::vector<glm::dvec3>> controlPoints;
    std::vector<std::vector<double>> weights;
    std::vector<double> knotsU, knotsV;
    bool isRational() const { return !weights.empty() && !weights[0].empty(); }
    double uMin = 0, uMax = 1, vMin = 0, vMax = 1;
};

class NURBSCurve {
public:
    int degree = 3;
    std::vector<glm::dvec3> controlPoints;
    std::vector<double> weights;
    std::vector<double> knots;
    bool isRational() const { return !weights.empty(); }
    double tMin = 0, tMax = 1;
    bool isPlanar = false;
    bool isClosed = false;
};

class Edge {
public:
    glm::dvec3 startPoint, endPoint;
    std::shared_ptr<NURBSCurve> curve;
    bool sameOrientation = true;
};

class Face {
public:
    std::vector<std::shared_ptr<Edge>> outerLoop;
    std::vector<std::vector<std::shared_ptr<Edge>>> innerLoops;
    std::shared_ptr<NURBSSurface> surface;
    bool sameSense = true;
    glm::vec3 color{0.8f, 0.8f, 0.8f};
};

class Body {
public:
    std::string name = "Body";
    std::vector<std::shared_ptr<Face>> faces;
    bool isSolid = true;
    glm::vec3 color{0.7f, 0.7f, 0.7f};
};

class Model {
public:
    std::string name = "Model";
    std::vector<std::shared_ptr<Body>> bodies;
};

STEPImporter::STEPImporter()
{
}

STEPImporter::~STEPImporter()
{
}

std::shared_ptr<Model> STEPImporter::importFile(const std::string& filename, const ImportOptions& options)
{
    m_options = options;
    m_entities.clear();
    m_points.clear();
    m_directions.clear();
    m_curves.clear();
    m_surfaces.clear();
    m_faces.clear();
    m_bodies.clear();
    m_colors.clear();
    m_errorMessage.clear();
    m_stats = ImportStats();
    
    // Parse file
    if (!parseFile(filename)) {
        return nullptr;
    }
    
    // Process entities
    if (!processEntities()) {
        return nullptr;
    }
    
    // Build model
    auto model = std::make_shared<Model>();
    model->name = filename;
    
    // Add all bodies to model
    for (auto& [id, body] : m_bodies) {
        model->bodies.push_back(body);
        m_stats.bodiesImported++;
    }
    
    // If no bodies found, try to find standalone faces
    if (model->bodies.empty() && !m_faces.empty()) {
        auto body = std::make_shared<Body>();
        body->name = "Imported Geometry";
        
        for (auto& [id, face] : m_faces) {
            body->faces.push_back(face);
        }
        
        model->bodies.push_back(body);
        m_stats.bodiesImported = 1;
    }
    
    return model;
}

bool STEPImporter::parseFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        m_errorMessage = "Failed to open file: " + filename;
        return false;
    }
    
    std::string line;
    std::string currentEntity;
    bool inDataSection = false;
    bool inHeader = false;
    
    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        // Check for section markers
        if (line.find("ISO-10303-21") != std::string::npos) {
            continue;
        }
        else if (line == "HEADER;") {
            inHeader = true;
            continue;
        }
        else if (line == "ENDSEC;" && inHeader) {
            inHeader = false;
            continue;
        }
        else if (line == "DATA;") {
            inDataSection = true;
            continue;
        }
        else if (line == "ENDSEC;" && inDataSection) {
            break;
        }
        else if (line.find("END-ISO-10303-21") != std::string::npos) {
            break;
        }
        
        if (inDataSection) {
            // Accumulate entity data (may span multiple lines)
            currentEntity += line;
            
            // Check if entity is complete (ends with ;)
            if (line.back() == ';') {
                ParsedSTEPEntity entity = parseEntity(currentEntity);
                if (entity.id > 0) {
                    m_entities[entity.id] = entity;
                    m_stats.totalEntities++;
                }
                currentEntity.clear();
            }
        }
    }
    
    file.close();
    return true;
}

ParsedSTEPEntity STEPImporter::parseEntity(const std::string& line)
{
    ParsedSTEPEntity entity;
    entity.id = 0;
    
    // HIGH FIX: Limit line length to prevent ReDoS attacks with crafted input
    constexpr size_t MAX_ENTITY_LINE_LENGTH = 1000000;  // 1MB max per entity line
    if (line.length() > MAX_ENTITY_LINE_LENGTH) {
        addWarning("Entity line too long (" + std::to_string(line.length()) + " chars), skipping");
        return entity;
    }
    
    // Parse entity ID: #123 = ENTITY_NAME(params);
    std::regex entityRegex(R"(#(\d+)\s*=\s*([A-Z_0-9]+)\s*\((.*)\)\s*;)");
    std::smatch match;
    
    if (std::regex_match(line, match, entityRegex)) {
        entity.id = std::stoi(match[1].str());
        entity.typeName = match[2].str();
        entity.rawData = match[3].str();
        entity.parameters = parseParameters(entity.rawData);
    }
    else {
        // Try complex entity format: #123 = (TYPE1(...) TYPE2(...));
        std::regex complexRegex(R"(#(\d+)\s*=\s*\((.*)\)\s*;)");
        if (std::regex_match(line, match, complexRegex)) {
            entity.id = std::stoi(match[1].str());
            entity.typeName = "COMPLEX";
            entity.rawData = match[2].str();
        }
    }
    
    return entity;
}

std::vector<std::string> STEPImporter::parseParameters(const std::string& data)
{
    std::vector<std::string> params;
    std::string current;
    int parenDepth = 0;
    bool inString = false;
    
    for (size_t i = 0; i < data.length(); i++) {
        char c = data[i];
        
        if (c == '\'' && !inString) {
            inString = true;
            current += c;
        }
        else if (c == '\'' && inString) {
            inString = false;
            current += c;
        }
        else if (inString) {
            current += c;
        }
        else if (c == '(') {
            parenDepth++;
            current += c;
        }
        else if (c == ')') {
            parenDepth--;
            current += c;
        }
        else if (c == ',' && parenDepth == 0) {
            // End of parameter
            params.push_back(current);
            current.clear();
        }
        else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            current += c;
        }
    }
    
    if (!current.empty()) {
        params.push_back(current);
    }
    
    return params;
}

bool STEPImporter::processEntities()
{
    // First pass: extract basic geometry
    for (auto& [id, entity] : m_entities) {
        if (entity.typeName == "CARTESIAN_POINT") {
            m_points[id] = getCartesianPoint(id);
        }
        else if (entity.typeName == "DIRECTION") {
            m_directions[id] = getDirection(id);
        }
    }
    
    // Second pass: extract surfaces and curves
    for (auto& [id, entity] : m_entities) {
        if (entity.typeName == "B_SPLINE_SURFACE_WITH_KNOTS" ||
            entity.typeName == "RATIONAL_B_SPLINE_SURFACE_WITH_KNOTS") {
            m_surfaces[id] = getBSplineSurface(id);
            m_stats.surfacesImported++;
        }
        else if (entity.typeName == "PLANE") {
            m_surfaces[id] = getPlane(id);
            m_stats.surfacesImported++;
        }
        else if (entity.typeName == "CYLINDRICAL_SURFACE") {
            m_surfaces[id] = getCylindricalSurface(id);
            m_stats.surfacesImported++;
        }
        else if (entity.typeName == "B_SPLINE_CURVE_WITH_KNOTS" ||
                 entity.typeName == "RATIONAL_B_SPLINE_CURVE_WITH_KNOTS") {
            m_curves[id] = getBSplineCurve(id);
            m_stats.curvesImported++;
        }
    }
    
    // Third pass: extract topology
    for (auto& [id, entity] : m_entities) {
        if (entity.typeName == "ADVANCED_FACE") {
            m_faces[id] = getAdvancedFace(id);
            m_stats.facesImported++;
        }
    }
    
    // Fourth pass: extract bodies
    for (auto& [id, entity] : m_entities) {
        if (entity.typeName == "MANIFOLD_SOLID_BREP") {
            m_bodies[id] = getManifoldSolidBrep(id);
        }
        else if (entity.typeName == "SHELL_BASED_SURFACE_MODEL") {
            m_bodies[id] = getShellBasedModel(id);
        }
    }
    
    // Process styled items for colors
    if (m_options.importColors) {
        processStyledItems();
    }
    
    return true;
}

glm::dvec3 STEPImporter::getCartesianPoint(int entityId)
{
    // Check cache
    auto it = m_points.find(entityId);
    if (it != m_points.end()) {
        return it->second;
    }
    
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return glm::dvec3(0);
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 2) {
        // Parameters: name, (x,y,z)
        std::string coordStr = entity.parameters[1];
        
        // Remove parentheses
        coordStr.erase(std::remove(coordStr.begin(), coordStr.end(), '('), coordStr.end());
        coordStr.erase(std::remove(coordStr.begin(), coordStr.end(), ')'), coordStr.end());
        
        auto coords = parseRealList(coordStr);
        if (coords.size() >= 3) {
            return glm::dvec3(coords[0], coords[1], coords[2]);
        }
    }
    
    return glm::dvec3(0);
}

glm::dvec3 STEPImporter::getDirection(int entityId)
{
    // Check cache
    auto it = m_directions.find(entityId);
    if (it != m_directions.end()) {
        return it->second;
    }
    
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return glm::dvec3(0, 0, 1);
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 2) {
        std::string dirStr = entity.parameters[1];
        dirStr.erase(std::remove(dirStr.begin(), dirStr.end(), '('), dirStr.end());
        dirStr.erase(std::remove(dirStr.begin(), dirStr.end(), ')'), dirStr.end());
        
        auto ratios = parseRealList(dirStr);
        if (ratios.size() >= 3) {
            return glm::normalize(glm::dvec3(ratios[0], ratios[1], ratios[2]));
        }
    }
    
    return glm::dvec3(0, 0, 1);
}

glm::dvec3 STEPImporter::getVector(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return glm::dvec3(0, 0, 1);
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 3) {
        int dirId = parseEntityRef(entity.parameters[1]);
        double magnitude = parseReal(entity.parameters[2]);
        glm::dvec3 dir = getDirection(dirId);
        return dir * magnitude;
    }
    
    return glm::dvec3(0, 0, 1);
}

glm::dmat4 STEPImporter::getAxis2Placement3D(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return glm::dmat4(1.0);
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 4) {
        int locationId = parseEntityRef(entity.parameters[1]);
        int axisId = parseEntityRef(entity.parameters[2]);
        int refDirId = parseEntityRef(entity.parameters[3]);
        
        glm::dvec3 origin = getCartesianPoint(locationId);
        glm::dvec3 zAxis = getDirection(axisId);
        glm::dvec3 xAxis = getDirection(refDirId);
        glm::dvec3 yAxis = glm::cross(zAxis, xAxis);
        
        glm::dmat4 matrix(1.0);
        matrix[0] = glm::dvec4(xAxis, 0.0);
        matrix[1] = glm::dvec4(yAxis, 0.0);
        matrix[2] = glm::dvec4(zAxis, 0.0);
        matrix[3] = glm::dvec4(origin, 1.0);
        
        return matrix;
    }
    
    return glm::dmat4(1.0);
}

std::shared_ptr<NURBSCurve> STEPImporter::getLine(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 3) {
        int pointId = parseEntityRef(entity.parameters[1]);
        int vectorId = parseEntityRef(entity.parameters[2]);
        
        glm::dvec3 point = getCartesianPoint(pointId);
        glm::dvec3 direction = getVector(vectorId);
        
        auto curve = std::make_shared<NURBSCurve>();
        curve->degree = 1;
        curve->controlPoints.push_back(point);
        curve->controlPoints.push_back(point + direction);
        curve->knots = {0, 0, 1, 1};
        
        return curve;
    }
    
    return nullptr;
}

std::shared_ptr<NURBSCurve> STEPImporter::getCircle(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 3) {
        int axisId = parseEntityRef(entity.parameters[1]);
        double radius = parseReal(entity.parameters[2]);
        
        glm::dmat4 placement = getAxis2Placement3D(axisId);
        glm::dvec3 center(placement[3]);
        glm::dvec3 xAxis(placement[0]);
        glm::dvec3 yAxis(placement[1]);
        
        // Create rational B-spline circle (exact representation)
        auto curve = std::make_shared<NURBSCurve>();
        curve->degree = 2;
        curve->isClosed = true;
        
        // 9 control points for a full circle
        double w = std::sqrt(2.0) / 2.0;
        curve->controlPoints = {
            center + radius * xAxis,
            center + radius * (xAxis + yAxis),
            center + radius * yAxis,
            center + radius * (-xAxis + yAxis),
            center - radius * xAxis,
            center + radius * (-xAxis - yAxis),
            center - radius * yAxis,
            center + radius * (xAxis - yAxis),
            center + radius * xAxis
        };
        
        curve->weights = {1, w, 1, w, 1, w, 1, w, 1};
        curve->knots = {0, 0, 0, 0.25, 0.25, 0.5, 0.5, 0.75, 0.75, 1, 1, 1};
        
        return curve;
    }
    
    return nullptr;
}

std::shared_ptr<NURBSCurve> STEPImporter::getBSplineCurve(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    
    // Parse B-spline curve parameters
    // Format: name, degree, control_points_list, curve_form, closed, self_intersect,
    //         knot_multiplicities, knots, knot_type, [weights for rational]
    
    if (entity.parameters.size() < 9) {
        return nullptr;
    }
    
    auto curve = std::make_shared<NURBSCurve>();
    curve->degree = parseInt(entity.parameters[1]);
    
    // Parse control point references
    auto cpRefs = parseEntityRefList(entity.parameters[2]);
    for (int ref : cpRefs) {
        curve->controlPoints.push_back(getCartesianPoint(ref));
    }
    
    // Parse knot multiplicities
    auto knotMults = parseIntList(entity.parameters[6]);
    
    // Parse knot values
    auto knotValues = parseRealList(entity.parameters[7]);
    
    // Build full knot vector
    for (size_t i = 0; i < knotValues.size() && i < knotMults.size(); i++) {
        for (int j = 0; j < knotMults[i]; j++) {
            curve->knots.push_back(knotValues[i]);
        }
    }
    
    // Check for rational (weights)
    if (entity.typeName.find("RATIONAL") != std::string::npos && entity.parameters.size() > 9) {
        curve->weights = parseRealList(entity.parameters[9]);
    }
    
    return curve;
}

std::shared_ptr<NURBSSurface> STEPImporter::getPlane(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 2) {
        int axisId = parseEntityRef(entity.parameters[1]);
        glm::dmat4 placement = getAxis2Placement3D(axisId);
        
        glm::dvec3 origin(placement[3]);
        glm::dvec3 xAxis(placement[0]);
        glm::dvec3 yAxis(placement[1]);
        
        // Create bilinear patch for plane
        auto surface = std::make_shared<NURBSSurface>();
        surface->degreeU = 1;
        surface->degreeV = 1;
        
        double size = 1000.0; // Arbitrary plane size
        surface->controlPoints = {
            {origin - size * xAxis - size * yAxis, origin + size * xAxis - size * yAxis},
            {origin - size * xAxis + size * yAxis, origin + size * xAxis + size * yAxis}
        };
        
        surface->knotsU = {0, 0, 1, 1};
        surface->knotsV = {0, 0, 1, 1};
        
        return surface;
    }
    
    return nullptr;
}

std::shared_ptr<NURBSSurface> STEPImporter::getCylindricalSurface(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 3) {
        int axisId = parseEntityRef(entity.parameters[1]);
        double radius = parseReal(entity.parameters[2]);
        
        glm::dmat4 placement = getAxis2Placement3D(axisId);
        glm::dvec3 origin(placement[3]);
        glm::dvec3 xAxis(placement[0]);
        glm::dvec3 yAxis(placement[1]);
        glm::dvec3 zAxis(placement[2]);
        
        auto surface = std::make_shared<NURBSSurface>();
        surface->degreeU = 2;
        surface->degreeV = 1;
        
        // Create NURBS representation of cylinder
        double height = 1000.0;
        double w = std::sqrt(2.0) / 2.0;
        
        // 9 control points in U (around), 2 in V (along axis)
        surface->controlPoints.resize(9);
        surface->weights.resize(9);
        
        std::vector<double> weights = {1, w, 1, w, 1, w, 1, w, 1};
        
        for (int i = 0; i < 9; i++) {
            double angle = i * M_PI / 4.0;
            glm::dvec3 radial = radius * (cos(angle) * xAxis + sin(angle) * yAxis);
            
            surface->controlPoints[i] = {
                origin + radial,
                origin + radial + height * zAxis
            };
            surface->weights[i] = {weights[i], weights[i]};
        }
        
        surface->knotsU = {0, 0, 0, 0.25, 0.25, 0.5, 0.5, 0.75, 0.75, 1, 1, 1};
        surface->knotsV = {0, 0, 1, 1};
        
        return surface;
    }
    
    return nullptr;
}

std::shared_ptr<NURBSSurface> STEPImporter::getBSplineSurface(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    
    // Parse B-spline surface parameters
    if (entity.parameters.size() < 12) {
        return nullptr;
    }
    
    auto surface = std::make_shared<NURBSSurface>();
    surface->degreeU = parseInt(entity.parameters[1]);
    surface->degreeV = parseInt(entity.parameters[2]);
    
    // Parse control points grid
    std::string cpGrid = entity.parameters[3];
    
    // Remove outer parentheses
    if (cpGrid.front() == '(' && cpGrid.back() == ')') {
        cpGrid = cpGrid.substr(1, cpGrid.length() - 2);
    }
    
    // Parse rows
    int depth = 0;
    std::string currentRow;
    for (char c : cpGrid) {
        if (c == '(') {
            if (depth == 0) currentRow.clear();
            depth++;
            if (depth > 1) currentRow += c;
        }
        else if (c == ')') {
            depth--;
            if (depth == 0) {
                // End of row
                auto refs = parseEntityRefList("(" + currentRow + ")");
                std::vector<glm::dvec3> row;
                for (int ref : refs) {
                    row.push_back(getCartesianPoint(ref));
                }
                surface->controlPoints.push_back(row);
            }
            else {
                currentRow += c;
            }
        }
        else if (depth > 0) {
            currentRow += c;
        }
    }
    
    // Parse knot multiplicities and knots
    auto knotMultsU = parseIntList(entity.parameters[8]);
    auto knotMultsV = parseIntList(entity.parameters[9]);
    auto knotValuesU = parseRealList(entity.parameters[10]);
    auto knotValuesV = parseRealList(entity.parameters[11]);
    
    // Build full knot vectors
    for (size_t i = 0; i < knotValuesU.size() && i < knotMultsU.size(); i++) {
        for (int j = 0; j < knotMultsU[i]; j++) {
            surface->knotsU.push_back(knotValuesU[i]);
        }
    }
    
    for (size_t i = 0; i < knotValuesV.size() && i < knotMultsV.size(); i++) {
        for (int j = 0; j < knotMultsV[i]; j++) {
            surface->knotsV.push_back(knotValuesV[i]);
        }
    }
    
    // Parse weights for rational surface
    if (entity.typeName.find("RATIONAL") != std::string::npos && entity.parameters.size() > 13) {
        // Parse weight grid similarly to control points
        std::string weightGrid = entity.parameters[13];
        // ... (similar parsing)
    }
    
    return surface;
}

std::shared_ptr<Face> STEPImporter::getAdvancedFace(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() < 4) {
        return nullptr;
    }
    
    auto face = std::make_shared<Face>();
    
    // Parse bounds list
    auto boundRefs = parseEntityRefList(entity.parameters[1]);
    
    for (int boundRef : boundRefs) {
        auto boundEntity = m_entities.find(boundRef);
        if (boundEntity != m_entities.end()) {
            bool isOuter = (boundEntity->second.typeName == "FACE_OUTER_BOUND");
            
            if (boundEntity->second.parameters.size() >= 2) {
                int loopRef = parseEntityRef(boundEntity->second.parameters[1]);
                auto edges = getEdgeLoop(loopRef);
                
                if (isOuter) {
                    face->outerLoop = edges;
                } else {
                    face->innerLoops.push_back(edges);
                }
            }
        }
    }
    
    // Get surface
    int surfaceRef = parseEntityRef(entity.parameters[2]);
    auto surfIt = m_surfaces.find(surfaceRef);
    if (surfIt != m_surfaces.end()) {
        face->surface = surfIt->second;
    }
    
    // Get same sense flag
    face->sameSense = parseBool(entity.parameters[3]);
    
    return face;
}

std::vector<std::shared_ptr<Edge>> STEPImporter::getEdgeLoop(int entityId)
{
    std::vector<std::shared_ptr<Edge>> edges;
    
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return edges;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() >= 2) {
        auto edgeRefs = parseEntityRefList(entity.parameters[1]);
        
        for (int edgeRef : edgeRefs) {
            auto edge = getEdgeCurveAsEdge(edgeRef);
            if (edge) {
                edges.push_back(edge);
            }
        }
    }
    
    return edges;
}

std::shared_ptr<Edge> STEPImporter::getEdgeCurveAsEdge(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    
    // Handle ORIENTED_EDGE
    if (entity.typeName == "ORIENTED_EDGE") {
        if (entity.parameters.size() >= 4) {
            int edgeCurveRef = parseEntityRef(entity.parameters[3]);
            bool orientation = parseBool(entity.parameters[4]);
            auto edge = getEdgeCurveAsEdge(edgeCurveRef);
            if (edge) {
                edge->sameOrientation = orientation;
            }
            return edge;
        }
    }
    
    // Handle EDGE_CURVE
    if (entity.typeName == "EDGE_CURVE" && entity.parameters.size() >= 5) {
        auto edge = std::make_shared<Edge>();
        
        int startVertexRef = parseEntityRef(entity.parameters[1]);
        int endVertexRef = parseEntityRef(entity.parameters[2]);
        int curveRef = parseEntityRef(entity.parameters[3]);
        edge->sameOrientation = parseBool(entity.parameters[4]);
        
        // Get vertex positions
        auto startVertexEntity = m_entities.find(startVertexRef);
        auto endVertexEntity = m_entities.find(endVertexRef);
        
        if (startVertexEntity != m_entities.end() && 
            startVertexEntity->second.parameters.size() >= 2) {
            int pointRef = parseEntityRef(startVertexEntity->second.parameters[1]);
            edge->startPoint = getCartesianPoint(pointRef);
        }
        
        if (endVertexEntity != m_entities.end() && 
            endVertexEntity->second.parameters.size() >= 2) {
            int pointRef = parseEntityRef(endVertexEntity->second.parameters[1]);
            edge->endPoint = getCartesianPoint(pointRef);
        }
        
        // Get curve
        auto curveIt = m_curves.find(curveRef);
        if (curveIt != m_curves.end()) {
            edge->curve = curveIt->second;
        } else {
            // Try to create curve from entity
            auto curveEntity = m_entities.find(curveRef);
            if (curveEntity != m_entities.end()) {
                if (curveEntity->second.typeName == "LINE") {
                    edge->curve = getLine(curveRef);
                } else if (curveEntity->second.typeName == "CIRCLE") {
                    edge->curve = getCircle(curveRef);
                }
            }
        }
        
        return edge;
    }
    
    return nullptr;
}

std::shared_ptr<Body> STEPImporter::getManifoldSolidBrep(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    if (entity.parameters.size() < 2) {
        return nullptr;
    }
    
    auto body = std::make_shared<Body>();
    body->name = parseString(entity.parameters[0]);
    body->isSolid = true;
    
    int shellRef = parseEntityRef(entity.parameters[1]);
    auto faceIds = getShellFaceIds(shellRef);
    
    for (int faceId : faceIds) {
        auto faceIt = m_faces.find(faceId);
        if (faceIt != m_faces.end()) {
            body->faces.push_back(faceIt->second);
        }
    }
    
    return body;
}

std::shared_ptr<Body> STEPImporter::getShellBasedModel(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt == m_entities.end()) {
        return nullptr;
    }
    
    auto& entity = entityIt->second;
    auto body = std::make_shared<Body>();
    body->name = parseString(entity.parameters[0]);
    body->isSolid = false;
    
    auto shellRefs = parseEntityRefList(entity.parameters[1]);
    for (int shellRef : shellRefs) {
        auto faceIds = getShellFaceIds(shellRef);
        for (int faceId : faceIds) {
            auto faceIt = m_faces.find(faceId);
            if (faceIt != m_faces.end()) {
                body->faces.push_back(faceIt->second);
            }
        }
    }
    
    return body;
}

std::vector<int> STEPImporter::getShellFaceIds(int shellId)
{
    std::vector<int> faceIds;
    
    auto entityIt = m_entities.find(shellId);
    if (entityIt != m_entities.end() && entityIt->second.parameters.size() >= 2) {
        faceIds = parseEntityRefList(entityIt->second.parameters[1]);
    }
    
    return faceIds;
}

void STEPImporter::processStyledItems()
{
    for (auto& [id, entity] : m_entities) {
        if (entity.typeName == "STYLED_ITEM" && entity.parameters.size() >= 3) {
            // Extract style and item references
            auto styleRefs = parseEntityRefList(entity.parameters[1]);
            int itemRef = parseEntityRef(entity.parameters[2]);
            
            // Find color in style chain
            for (int styleRef : styleRefs) {
                // Traverse style hierarchy to find color
                // This is simplified - full implementation would follow all references
            }
        }
    }
}

glm::vec3 STEPImporter::getColorRGB(int entityId)
{
    auto entityIt = m_entities.find(entityId);
    if (entityIt != m_entities.end() && entityIt->second.parameters.size() >= 4) {
        auto& params = entityIt->second.parameters;
        return glm::vec3(
            parseReal(params[1]),
            parseReal(params[2]),
            parseReal(params[3])
        );
    }
    return glm::vec3(0.7f);
}

int STEPImporter::parseEntityRef(const std::string& ref)
{
    if (ref.empty() || ref[0] != '#') {
        return 0;
    }
    try {
        return std::stoi(ref.substr(1));
    } catch (...) {
        return 0;
    }
}

double STEPImporter::parseReal(const std::string& str)
{
    try {
        return std::stod(str);
    } catch (...) {
        return 0.0;
    }
}

int STEPImporter::parseInt(const std::string& str)
{
    try {
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

std::string STEPImporter::parseString(const std::string& str)
{
    std::string result = str;
    
    // Remove quotes
    if (result.size() >= 2 && result.front() == '\'' && result.back() == '\'') {
        result = result.substr(1, result.size() - 2);
    }
    
    // Unescape doubled quotes
    size_t pos = 0;
    while ((pos = result.find("''", pos)) != std::string::npos) {
        result.replace(pos, 2, "'");
        pos += 1;
    }
    
    return result;
}

bool STEPImporter::parseBool(const std::string& str)
{
    return str == ".T." || str == "T" || str == "TRUE" || str == "true";
}

std::vector<int> STEPImporter::parseIntList(const std::string& str)
{
    std::vector<int> result;
    std::string s = str;
    
    // Remove parentheses
    s.erase(std::remove(s.begin(), s.end(), '('), s.end());
    s.erase(std::remove(s.begin(), s.end(), ')'), s.end());
    
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(parseInt(item));
    }
    
    return result;
}

std::vector<double> STEPImporter::parseRealList(const std::string& str)
{
    std::vector<double> result;
    std::string s = str;
    
    s.erase(std::remove(s.begin(), s.end(), '('), s.end());
    s.erase(std::remove(s.begin(), s.end(), ')'), s.end());
    
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(parseReal(item));
    }
    
    return result;
}

std::vector<int> STEPImporter::parseEntityRefList(const std::string& str)
{
    std::vector<int> result;
    std::string s = str;
    
    s.erase(std::remove(s.begin(), s.end(), '('), s.end());
    s.erase(std::remove(s.begin(), s.end(), ')'), s.end());
    
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        int ref = parseEntityRef(item);
        if (ref > 0) {
            result.push_back(ref);
        }
    }
    
    return result;
}

void STEPImporter::addWarning(const std::string& msg)
{
    m_stats.warnings.push_back(msg);
}

} // namespace dc
