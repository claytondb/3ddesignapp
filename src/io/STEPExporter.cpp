#include "STEPExporter.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace dc {

// Placeholder classes for compilation - replace with actual implementations
class NURBSSurface {
public:
    int degreeU = 3, degreeV = 3;
    std::vector<std::vector<glm::dvec3>> controlPoints;
    std::vector<std::vector<double>> weights;
    std::vector<double> knotsU, knotsV;
    bool isRational() const { return !weights.empty(); }
};

class NURBSCurve {
public:
    int degree = 3;
    std::vector<glm::dvec3> controlPoints;
    std::vector<double> weights;
    std::vector<double> knots;
    bool isRational() const { return !weights.empty(); }
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

STEPExporter::STEPExporter()
    : m_nextEntityId(1)
{
}

STEPExporter::~STEPExporter()
{
    if (m_file.is_open()) {
        m_file.close();
    }
}

bool STEPExporter::exportModel(const Model& model, const std::string& filename, const ExportOptions& options)
{
    m_options = options;
    m_nextEntityId = 1;
    m_entities.clear();
    m_entityMap.clear();
    m_errorMessage.clear();
    
    m_file.open(filename, std::ios::out);
    if (!m_file.is_open()) {
        m_errorMessage = "Failed to open file for writing: " + filename;
        return false;
    }
    
    try {
        writeHeader(filename);
        
        // Export all bodies
        std::vector<int> shapeIds;
        for (const auto& body : model.bodies) {
            int bodyId = exportSolidBrep(*body);
            if (bodyId > 0) {
                shapeIds.push_back(bodyId);
                
                // Add styling for AP214
                if (m_options.format == ExportFormat::STEP_AP214 && m_options.includeColors) {
                    int colorId = exportColorRGB(body->color);
                    int styleId = exportSurfaceStyle(colorId);
                    exportStyledItem(bodyId, styleId);
                }
            }
        }
        
        // Create product structure
        int productId = exportProduct(model.name, "Exported from " + m_options.applicationName);
        int productDefId = exportProductDefinition(productId);
        int shapeRepId = exportShapeRepresentation(shapeIds);
        
        // Link shape to product
        std::ostringstream ss;
        ss << "SHAPE_DEFINITION_REPRESENTATION(#" << productDefId << ",#" << shapeRepId << ")";
        addEntity(STEPEntityType::SHAPE_DEFINITION_REPRESENTATION, ss.str());
        
        writeDataSection();
        writeFooter();
        
        m_file.close();
        return true;
    }
    catch (const std::exception& e) {
        m_errorMessage = std::string("Export error: ") + e.what();
        m_file.close();
        return false;
    }
}

bool STEPExporter::exportAssembly(const std::vector<std::shared_ptr<Body>>& bodies,
                                   const std::string& filename,
                                   const ExportOptions& options)
{
    Model model;
    model.name = "Assembly";
    model.bodies = bodies;
    return exportModel(model, filename, options);
}

size_t STEPExporter::estimateFileSize(const Model& model, const ExportOptions& options) const
{
    // Rough estimation based on geometry complexity
    size_t estimate = 2000; // Header/footer overhead
    
    for (const auto& body : model.bodies) {
        estimate += 500; // Per-body overhead
        estimate += body->faces.size() * 2000; // Per-face estimate
    }
    
    return estimate;
}

void STEPExporter::writeHeader(const std::string& filename)
{
    std::string schema = (m_options.format == ExportFormat::STEP_AP203) 
        ? "CONFIG_CONTROL_DESIGN" 
        : "AUTOMOTIVE_DESIGN";
    
    m_file << "ISO-10303-21;\n";
    m_file << "HEADER;\n";
    m_file << "FILE_DESCRIPTION(('STEP AP214 Model'),'2;1');\n";
    m_file << "FILE_NAME('" << escapeString(filename) << "','" 
           << getCurrentTimestamp() << "',('" 
           << escapeString(m_options.authorName.empty() ? "Unknown" : m_options.authorName) 
           << "'),('" 
           << escapeString(m_options.organizationName.empty() ? "Unknown" : m_options.organizationName) 
           << "'),'" << m_options.applicationName << " " << m_options.applicationVersion 
           << "','" << m_options.applicationName << "','');\n";
    m_file << "FILE_SCHEMA(('" << schema << "'));\n";
    m_file << "ENDSEC;\n";
}

void STEPExporter::writeDataSection()
{
    m_file << "DATA;\n";
    
    for (const auto& entity : m_entities) {
        m_file << "#" << entity.id << "=" << entity.data << ";\n";
    }
    
    m_file << "ENDSEC;\n";
}

void STEPExporter::writeFooter()
{
    m_file << "END-ISO-10303-21;\n";
}

int STEPExporter::addEntity(STEPEntityType type, const std::string& data)
{
    int id = m_nextEntityId++;
    m_entities.emplace_back(id, type, data);
    return id;
}

int STEPExporter::getOrCreateEntity(void* object, STEPEntityType type, const std::string& data)
{
    auto it = m_entityMap.find(object);
    if (it != m_entityMap.end()) {
        return it->second;
    }
    
    int id = addEntity(type, data);
    m_entityMap[object] = id;
    return id;
}

int STEPExporter::exportCartesianPoint(const glm::dvec3& point)
{
    glm::dvec3 p = transformPoint(point);
    std::ostringstream ss;
    ss << "CARTESIAN_POINT(''," << formatPoint(p) << ")";
    return addEntity(STEPEntityType::CARTESIAN_POINT, ss.str());
}

int STEPExporter::exportDirection(const glm::dvec3& dir)
{
    glm::dvec3 d = transformDirection(glm::normalize(dir));
    std::ostringstream ss;
    ss << "DIRECTION(''," << formatDirection(d) << ")";
    return addEntity(STEPEntityType::DIRECTION, ss.str());
}

int STEPExporter::exportVector(const glm::dvec3& dir, double magnitude)
{
    int dirId = exportDirection(dir);
    std::ostringstream ss;
    ss << "VECTOR('',#" << dirId << "," << formatReal(magnitude) << ")";
    return addEntity(STEPEntityType::VECTOR, ss.str());
}

int STEPExporter::exportAxis2Placement3D(const glm::dvec3& origin, const glm::dvec3& zDir, const glm::dvec3& xDir)
{
    int originId = exportCartesianPoint(origin);
    int zDirId = exportDirection(zDir);
    int xDirId = exportDirection(xDir);
    
    std::ostringstream ss;
    ss << "AXIS2_PLACEMENT_3D('',#" << originId << ",#" << zDirId << ",#" << xDirId << ")";
    return addEntity(STEPEntityType::AXIS2_PLACEMENT_3D, ss.str());
}

int STEPExporter::exportLine(const glm::dvec3& start, const glm::dvec3& dir)
{
    int pointId = exportCartesianPoint(start);
    int vectorId = exportVector(dir, glm::length(dir));
    
    std::ostringstream ss;
    ss << "LINE('',#" << pointId << ",#" << vectorId << ")";
    return addEntity(STEPEntityType::LINE, ss.str());
}

int STEPExporter::exportCircle(const glm::dvec3& center, const glm::dvec3& normal, double radius)
{
    // Create coordinate system for circle
    glm::dvec3 xDir = glm::abs(normal.x) < 0.9 
        ? glm::normalize(glm::cross(normal, glm::dvec3(1, 0, 0)))
        : glm::normalize(glm::cross(normal, glm::dvec3(0, 1, 0)));
    
    int axisId = exportAxis2Placement3D(center, normal, xDir);
    
    std::ostringstream ss;
    ss << "CIRCLE('',#" << axisId << "," << formatReal(radius * m_options.getUnitScale()) << ")";
    return addEntity(STEPEntityType::CIRCLE, ss.str());
}

int STEPExporter::exportBSplineCurve(const NURBSCurve& curve)
{
    std::ostringstream ss;
    
    // Export control points
    std::vector<int> pointIds;
    for (const auto& cp : curve.controlPoints) {
        pointIds.push_back(exportCartesianPoint(cp));
    }
    
    // Build control points list
    std::ostringstream cpList;
    cpList << "(";
    for (size_t i = 0; i < pointIds.size(); i++) {
        if (i > 0) cpList << ",";
        cpList << "#" << pointIds[i];
    }
    cpList << ")";
    
    // Build knot multiplicities
    std::vector<int> knotMults;
    std::vector<double> uniqueKnots;
    
    if (!curve.knots.empty()) {
        uniqueKnots.push_back(curve.knots[0]);
        knotMults.push_back(1);
        
        for (size_t i = 1; i < curve.knots.size(); i++) {
            if (std::abs(curve.knots[i] - uniqueKnots.back()) < 1e-10) {
                knotMults.back()++;
            } else {
                uniqueKnots.push_back(curve.knots[i]);
                knotMults.push_back(1);
            }
        }
    }
    
    // Build knot multiplicity list
    std::ostringstream multList;
    multList << "(";
    for (size_t i = 0; i < knotMults.size(); i++) {
        if (i > 0) multList << ",";
        multList << knotMults[i];
    }
    multList << ")";
    
    // Build knot list
    std::ostringstream knotList;
    knotList << "(";
    for (size_t i = 0; i < uniqueKnots.size(); i++) {
        if (i > 0) knotList << ",";
        knotList << formatReal(uniqueKnots[i]);
    }
    knotList << ")";
    
    if (curve.isRational()) {
        // Build weights list
        std::ostringstream weightList;
        weightList << "(";
        for (size_t i = 0; i < curve.weights.size(); i++) {
            if (i > 0) weightList << ",";
            weightList << formatReal(curve.weights[i]);
        }
        weightList << ")";
        
        ss << "RATIONAL_B_SPLINE_CURVE_WITH_KNOTS(''," 
           << curve.degree << "," << cpList.str() 
           << ",.UNSPECIFIED.,.F.,.F.," 
           << multList.str() << "," << knotList.str() 
           << ",.UNSPECIFIED.," << weightList.str() << ")";
    } else {
        ss << "B_SPLINE_CURVE_WITH_KNOTS(''," 
           << curve.degree << "," << cpList.str() 
           << ",.UNSPECIFIED.,.F.,.F.," 
           << multList.str() << "," << knotList.str() 
           << ",.UNSPECIFIED.)";
    }
    
    return addEntity(STEPEntityType::B_SPLINE_CURVE_WITH_KNOTS, ss.str());
}

int STEPExporter::exportPlane(const glm::dvec3& origin, const glm::dvec3& normal)
{
    glm::dvec3 xDir = glm::abs(normal.x) < 0.9 
        ? glm::normalize(glm::cross(normal, glm::dvec3(1, 0, 0)))
        : glm::normalize(glm::cross(normal, glm::dvec3(0, 1, 0)));
    
    int axisId = exportAxis2Placement3D(origin, normal, xDir);
    
    std::ostringstream ss;
    ss << "PLANE('',#" << axisId << ")";
    return addEntity(STEPEntityType::PLANE, ss.str());
}

int STEPExporter::exportCylindricalSurface(const glm::dvec3& origin, const glm::dvec3& axis, double radius)
{
    glm::dvec3 xDir = glm::abs(axis.x) < 0.9 
        ? glm::normalize(glm::cross(axis, glm::dvec3(1, 0, 0)))
        : glm::normalize(glm::cross(axis, glm::dvec3(0, 1, 0)));
    
    int axisId = exportAxis2Placement3D(origin, axis, xDir);
    
    std::ostringstream ss;
    ss << "CYLINDRICAL_SURFACE('',#" << axisId << "," << formatReal(radius * m_options.getUnitScale()) << ")";
    return addEntity(STEPEntityType::CYLINDRICAL_SURFACE, ss.str());
}

int STEPExporter::exportBSplineSurface(const NURBSSurface& surface)
{
    std::ostringstream ss;
    
    // Export control points grid
    std::ostringstream cpGrid;
    cpGrid << "(";
    for (size_t i = 0; i < surface.controlPoints.size(); i++) {
        if (i > 0) cpGrid << ",";
        cpGrid << "(";
        for (size_t j = 0; j < surface.controlPoints[i].size(); j++) {
            if (j > 0) cpGrid << ",";
            int ptId = exportCartesianPoint(surface.controlPoints[i][j]);
            cpGrid << "#" << ptId;
        }
        cpGrid << ")";
    }
    cpGrid << ")";
    
    // Process knots for U direction
    std::vector<int> knotMultsU;
    std::vector<double> uniqueKnotsU;
    if (!surface.knotsU.empty()) {
        uniqueKnotsU.push_back(surface.knotsU[0]);
        knotMultsU.push_back(1);
        for (size_t i = 1; i < surface.knotsU.size(); i++) {
            if (std::abs(surface.knotsU[i] - uniqueKnotsU.back()) < 1e-10) {
                knotMultsU.back()++;
            } else {
                uniqueKnotsU.push_back(surface.knotsU[i]);
                knotMultsU.push_back(1);
            }
        }
    }
    
    // Process knots for V direction
    std::vector<int> knotMultsV;
    std::vector<double> uniqueKnotsV;
    if (!surface.knotsV.empty()) {
        uniqueKnotsV.push_back(surface.knotsV[0]);
        knotMultsV.push_back(1);
        for (size_t i = 1; i < surface.knotsV.size(); i++) {
            if (std::abs(surface.knotsV[i] - uniqueKnotsV.back()) < 1e-10) {
                knotMultsV.back()++;
            } else {
                uniqueKnotsV.push_back(surface.knotsV[i]);
                knotMultsV.push_back(1);
            }
        }
    }
    
    // Format knot multiplicities and values
    auto formatIntList = [](const std::vector<int>& v) {
        std::ostringstream os;
        os << "(";
        for (size_t i = 0; i < v.size(); i++) {
            if (i > 0) os << ",";
            os << v[i];
        }
        os << ")";
        return os.str();
    };
    
    auto formatDoubleList = [this](const std::vector<double>& v) {
        std::ostringstream os;
        os << "(";
        for (size_t i = 0; i < v.size(); i++) {
            if (i > 0) os << ",";
            os << formatReal(v[i]);
        }
        os << ")";
        return os.str();
    };
    
    if (surface.isRational()) {
        // Build weights grid
        std::ostringstream weightGrid;
        weightGrid << "(";
        for (size_t i = 0; i < surface.weights.size(); i++) {
            if (i > 0) weightGrid << ",";
            weightGrid << "(";
            for (size_t j = 0; j < surface.weights[i].size(); j++) {
                if (j > 0) weightGrid << ",";
                weightGrid << formatReal(surface.weights[i][j]);
            }
            weightGrid << ")";
        }
        weightGrid << ")";
        
        ss << "RATIONAL_B_SPLINE_SURFACE_WITH_KNOTS('',";
    } else {
        ss << "B_SPLINE_SURFACE_WITH_KNOTS('',";
    }
    
    ss << surface.degreeU << "," << surface.degreeV << "," << cpGrid.str() 
       << ",.UNSPECIFIED.,.F.,.F.,.F.,"
       << formatIntList(knotMultsU) << "," << formatIntList(knotMultsV) << ","
       << formatDoubleList(uniqueKnotsU) << "," << formatDoubleList(uniqueKnotsV)
       << ",.UNSPECIFIED.";
    
    if (surface.isRational()) {
        // Add weights for rational surface
        std::ostringstream weightGrid;
        weightGrid << "(";
        for (size_t i = 0; i < surface.weights.size(); i++) {
            if (i > 0) weightGrid << ",";
            weightGrid << "(";
            for (size_t j = 0; j < surface.weights[i].size(); j++) {
                if (j > 0) weightGrid << ",";
                weightGrid << formatReal(surface.weights[i][j]);
            }
            weightGrid << ")";
        }
        weightGrid << ")";
        ss << "," << weightGrid.str();
    }
    
    ss << ")";
    
    return addEntity(STEPEntityType::B_SPLINE_SURFACE_WITH_KNOTS, ss.str());
}

int STEPExporter::exportVertexPoint(const glm::dvec3& point)
{
    int pointId = exportCartesianPoint(point);
    
    std::ostringstream ss;
    ss << "VERTEX_POINT('',#" << pointId << ")";
    return addEntity(STEPEntityType::VERTEX_POINT, ss.str());
}

int STEPExporter::exportEdgeCurve(const Edge& edge)
{
    int startVertexId = exportVertexPoint(edge.startPoint);
    int endVertexId = exportVertexPoint(edge.endPoint);
    
    int curveId;
    if (edge.curve) {
        curveId = exportBSplineCurve(*edge.curve);
    } else {
        // Create a line
        glm::dvec3 dir = edge.endPoint - edge.startPoint;
        curveId = exportLine(edge.startPoint, dir);
    }
    
    std::ostringstream ss;
    ss << "EDGE_CURVE('',#" << startVertexId << ",#" << endVertexId 
       << ",#" << curveId << "," << (edge.sameOrientation ? ".T." : ".F.") << ")";
    return addEntity(STEPEntityType::EDGE_CURVE, ss.str());
}

int STEPExporter::exportEdgeLoop(const std::vector<int>& edgeIds)
{
    std::ostringstream ss;
    ss << "EDGE_LOOP('',(";
    for (size_t i = 0; i < edgeIds.size(); i++) {
        if (i > 0) ss << ",";
        ss << "#" << edgeIds[i];
    }
    ss << "))";
    return addEntity(STEPEntityType::EDGE_LOOP, ss.str());
}

int STEPExporter::exportFace(const Face& face)
{
    // Export surface geometry
    int surfaceId;
    if (face.surface) {
        surfaceId = exportBSplineSurface(*face.surface);
    } else {
        // Default to plane at origin
        surfaceId = exportPlane(glm::dvec3(0), glm::dvec3(0, 0, 1));
    }
    
    // Export outer loop
    std::vector<int> outerEdgeIds;
    for (const auto& edge : face.outerLoop) {
        outerEdgeIds.push_back(exportEdgeCurve(*edge));
    }
    int outerLoopId = exportEdgeLoop(outerEdgeIds);
    
    std::ostringstream outerBoundSS;
    outerBoundSS << "FACE_OUTER_BOUND('',#" << outerLoopId << ",.T.)";
    int outerBoundId = addEntity(STEPEntityType::FACE_OUTER_BOUND, outerBoundSS.str());
    
    // Export inner loops (holes)
    std::vector<int> boundIds;
    boundIds.push_back(outerBoundId);
    
    for (const auto& innerLoop : face.innerLoops) {
        std::vector<int> innerEdgeIds;
        for (const auto& edge : innerLoop) {
            innerEdgeIds.push_back(exportEdgeCurve(*edge));
        }
        int innerLoopId = exportEdgeLoop(innerEdgeIds);
        
        std::ostringstream innerBoundSS;
        innerBoundSS << "FACE_BOUND('',#" << innerLoopId << ",.T.)";
        boundIds.push_back(addEntity(STEPEntityType::FACE_BOUND, innerBoundSS.str()));
    }
    
    // Create advanced face
    std::ostringstream ss;
    ss << "ADVANCED_FACE('',(";
    for (size_t i = 0; i < boundIds.size(); i++) {
        if (i > 0) ss << ",";
        ss << "#" << boundIds[i];
    }
    ss << "),#" << surfaceId << "," << (face.sameSense ? ".T." : ".F.") << ")";
    
    return addEntity(STEPEntityType::ADVANCED_FACE, ss.str());
}

int STEPExporter::exportShell(const std::vector<int>& faceIds, bool closed)
{
    std::ostringstream ss;
    
    if (closed) {
        ss << "CLOSED_SHELL('',(";
    } else {
        ss << "OPEN_SHELL('',(";
    }
    
    for (size_t i = 0; i < faceIds.size(); i++) {
        if (i > 0) ss << ",";
        ss << "#" << faceIds[i];
    }
    ss << "))";
    
    return addEntity(closed ? STEPEntityType::CLOSED_SHELL : STEPEntityType::OPEN_SHELL, ss.str());
}

int STEPExporter::exportSolidBrep(const Body& body)
{
    // Export all faces
    std::vector<int> faceIds;
    for (const auto& face : body.faces) {
        faceIds.push_back(exportFace(*face));
    }
    
    // Create shell
    int shellId = exportShell(faceIds, body.isSolid);
    
    if (body.isSolid) {
        // Create manifold solid BREP
        std::ostringstream ss;
        ss << "MANIFOLD_SOLID_BREP('" << escapeString(body.name) << "',#" << shellId << ")";
        return addEntity(STEPEntityType::MANIFOLD_SOLID_BREP, ss.str());
    } else {
        // Return shell-based surface model
        std::ostringstream ss;
        ss << "SHELL_BASED_SURFACE_MODEL('" << escapeString(body.name) << "',(#" << shellId << "))";
        return addEntity(STEPEntityType::SHELL_BASED_SURFACE_MODEL, ss.str());
    }
}

int STEPExporter::exportProduct(const std::string& name, const std::string& description)
{
    std::ostringstream ss;
    ss << "PRODUCT('" << escapeString(name) << "','" << escapeString(name) 
       << "','" << escapeString(description) << "',())";
    return addEntity(STEPEntityType::PRODUCT, ss.str());
}

int STEPExporter::exportProductDefinition(int productId)
{
    // Product definition formation
    std::ostringstream formationSS;
    formationSS << "PRODUCT_DEFINITION_FORMATION('','',#" << productId << ")";
    int formationId = addEntity(STEPEntityType::PRODUCT_DEFINITION_FORMATION, formationSS.str());
    
    // Product definition
    std::ostringstream defSS;
    defSS << "PRODUCT_DEFINITION('design','',#" << formationId << ",$)";
    int defId = addEntity(STEPEntityType::PRODUCT_DEFINITION, defSS.str());
    
    // Product definition shape
    std::ostringstream shapeSS;
    shapeSS << "PRODUCT_DEFINITION_SHAPE('','',#" << defId << ")";
    return addEntity(STEPEntityType::PRODUCT_DEFINITION_SHAPE, shapeSS.str());
}

int STEPExporter::exportShapeRepresentation(const std::vector<int>& itemIds)
{
    // Create context
    int originId = exportCartesianPoint(glm::dvec3(0));
    int zDirId = exportDirection(glm::dvec3(0, 0, 1));
    int xDirId = exportDirection(glm::dvec3(1, 0, 0));
    
    std::ostringstream axisSS;
    axisSS << "AXIS2_PLACEMENT_3D('',#" << originId << ",#" << zDirId << ",#" << xDirId << ")";
    int axisId = addEntity(STEPEntityType::AXIS2_PLACEMENT_3D, axisSS.str());
    
    // Create representation context
    std::ostringstream ctxSS;
    ctxSS << "(GEOMETRIC_REPRESENTATION_CONTEXT(3)"
          << "GLOBAL_UNCERTAINTY_ASSIGNED_CONTEXT"
          << "((LENGTH_MEASURE(1.E-05)#0))"
          << "GLOBAL_UNIT_ASSIGNED_CONTEXT"
          << "((LENGTH_UNIT()NAMED_UNIT(#0)SI_UNIT(.MILLI.,.METRE.))"
          << "(NAMED_UNIT(#0)PLANE_ANGLE_UNIT()SI_UNIT($,.RADIAN.))"
          << "(NAMED_UNIT(#0)SI_UNIT($,.STERADIAN.)SOLID_ANGLE_UNIT()))"
          << "REPRESENTATION_CONTEXT('',''))";
    int contextId = addEntity(STEPEntityType::SHAPE_REPRESENTATION, ctxSS.str());
    
    // Create shape representation
    std::ostringstream ss;
    ss << "SHAPE_REPRESENTATION('',(#" << axisId;
    for (int id : itemIds) {
        ss << ",#" << id;
    }
    ss << "),#" << contextId << ")";
    
    return addEntity(STEPEntityType::SHAPE_REPRESENTATION, ss.str());
}

int STEPExporter::exportColorRGB(const glm::vec3& color)
{
    std::ostringstream ss;
    ss << "COLOUR_RGB(''," << formatReal(color.r) << "," 
       << formatReal(color.g) << "," << formatReal(color.b) << ")";
    return addEntity(STEPEntityType::COLOUR_RGB, ss.str());
}

int STEPExporter::exportSurfaceStyle(int colorId)
{
    // Surface style fill area
    std::ostringstream fillSS;
    fillSS << "FILL_AREA_STYLE_COLOUR('',#" << colorId << ")";
    int fillId = addEntity(STEPEntityType::SURFACE_STYLE_USAGE, fillSS.str());
    
    // Fill area style
    std::ostringstream areaSS;
    areaSS << "FILL_AREA_STYLE('',(#" << fillId << "))";
    int areaId = addEntity(STEPEntityType::SURFACE_STYLE_USAGE, areaSS.str());
    
    // Surface side style
    std::ostringstream sideSS;
    sideSS << "SURFACE_SIDE_STYLE('',(#" << areaId << "))";
    int sideId = addEntity(STEPEntityType::SURFACE_STYLE_USAGE, sideSS.str());
    
    // Surface style usage
    std::ostringstream usageSS;
    usageSS << "SURFACE_STYLE_USAGE(.BOTH.,#" << sideId << ")";
    return addEntity(STEPEntityType::SURFACE_STYLE_USAGE, usageSS.str());
}

int STEPExporter::exportStyledItem(int itemId, int styleId)
{
    // Presentation style assignment
    std::ostringstream psaSS;
    psaSS << "PRESENTATION_STYLE_ASSIGNMENT((#" << styleId << "))";
    int psaId = addEntity(STEPEntityType::STYLED_ITEM, psaSS.str());
    
    // Styled item
    std::ostringstream ss;
    ss << "STYLED_ITEM('',(#" << psaId << "),#" << itemId << ")";
    return addEntity(STEPEntityType::STYLED_ITEM, ss.str());
}

std::string STEPExporter::formatReal(double value) const
{
    std::ostringstream ss;
    ss << std::setprecision(15) << std::scientific << value;
    std::string result = ss.str();
    
    // STEP format requires specific exponential notation
    size_t ePos = result.find('e');
    if (ePos != std::string::npos) {
        result.replace(ePos, 1, "E");
    }
    
    return result;
}

std::string STEPExporter::formatPoint(const glm::dvec3& p) const
{
    std::ostringstream ss;
    double scale = m_options.getUnitScale();
    ss << "(" << formatReal(p.x * scale) << "," 
       << formatReal(p.y * scale) << "," 
       << formatReal(p.z * scale) << ")";
    return ss.str();
}

std::string STEPExporter::formatDirection(const glm::dvec3& d) const
{
    std::ostringstream ss;
    ss << "(" << formatReal(d.x) << "," << formatReal(d.y) << "," << formatReal(d.z) << ")";
    return ss.str();
}

std::string STEPExporter::escapeString(const std::string& str) const
{
    std::string result;
    result.reserve(str.size());
    
    for (char c : str) {
        if (c == '\'') {
            result += "''";
        } else if (c == '\\') {
            result += "\\\\";
        } else {
            result += c;
        }
    }
    
    return result;
}

std::string STEPExporter::getEntityTypeName(STEPEntityType type) const
{
    switch (type) {
        case STEPEntityType::CARTESIAN_POINT: return "CARTESIAN_POINT";
        case STEPEntityType::DIRECTION: return "DIRECTION";
        case STEPEntityType::VECTOR: return "VECTOR";
        case STEPEntityType::AXIS2_PLACEMENT_3D: return "AXIS2_PLACEMENT_3D";
        case STEPEntityType::LINE: return "LINE";
        case STEPEntityType::CIRCLE: return "CIRCLE";
        case STEPEntityType::B_SPLINE_CURVE_WITH_KNOTS: return "B_SPLINE_CURVE_WITH_KNOTS";
        case STEPEntityType::B_SPLINE_SURFACE_WITH_KNOTS: return "B_SPLINE_SURFACE_WITH_KNOTS";
        case STEPEntityType::PLANE: return "PLANE";
        case STEPEntityType::CYLINDRICAL_SURFACE: return "CYLINDRICAL_SURFACE";
        case STEPEntityType::ADVANCED_FACE: return "ADVANCED_FACE";
        case STEPEntityType::CLOSED_SHELL: return "CLOSED_SHELL";
        case STEPEntityType::MANIFOLD_SOLID_BREP: return "MANIFOLD_SOLID_BREP";
        default: return "UNKNOWN";
    }
}

std::string STEPExporter::getCurrentTimestamp() const
{
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    
    std::ostringstream ss;
    ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

glm::dvec3 STEPExporter::transformPoint(const glm::dvec3& point) const
{
    glm::dmat4 transform = m_options.getCoordinateTransform();
    glm::dvec4 p4(point, 1.0);
    glm::dvec4 result = transform * p4;
    return glm::dvec3(result);
}

glm::dvec3 STEPExporter::transformDirection(const glm::dvec3& dir) const
{
    glm::dmat4 transform = m_options.getCoordinateTransform();
    glm::dvec4 d4(dir, 0.0);
    glm::dvec4 result = transform * d4;
    return glm::normalize(glm::dvec3(result));
}

} // namespace dc
