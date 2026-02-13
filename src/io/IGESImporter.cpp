#include "IGESImporter.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cctype>

namespace dc {

// Placeholder classes
class NURBSSurface {
public:
    int degreeU = 3, degreeV = 3;
    std::vector<std::vector<glm::dvec3>> controlPoints;
    std::vector<std::vector<double>> weights;
    std::vector<double> knotsU, knotsV;
    double uMin = 0, uMax = 1, vMin = 0, vMax = 1;
};

class NURBSCurve {
public:
    int degree = 3;
    std::vector<glm::dvec3> controlPoints;
    std::vector<double> weights;
    std::vector<double> knots;
    double tMin = 0, tMax = 1;
    bool isPlanar = false;
    bool isClosed = false;
};

class Edge {
public:
    glm::dvec3 startPoint, endPoint;
    std::shared_ptr<NURBSCurve> curve;
};

class Face {
public:
    std::vector<std::shared_ptr<Edge>> outerLoop;
    std::vector<std::vector<std::shared_ptr<Edge>>> innerLoops;
    std::shared_ptr<NURBSSurface> surface;
    glm::vec3 color{0.8f, 0.8f, 0.8f};
};

class Body {
public:
    std::string name = "Body";
    std::vector<std::shared_ptr<Face>> faces;
    glm::vec3 color{0.7f, 0.7f, 0.7f};
};

class Model {
public:
    std::string name = "Model";
    std::vector<std::shared_ptr<Body>> bodies;
};

IGESImporter::IGESImporter()
{
}

IGESImporter::~IGESImporter()
{
}

std::shared_ptr<Model> IGESImporter::importFile(const std::string& filename, const ImportOptions& options)
{
    m_options = options;
    m_directoryEntries.clear();
    m_parameterEntries.clear();
    m_points.clear();
    m_curves.clear();
    m_surfaces.clear();
    m_transformations.clear();
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
    model->name = m_productId.empty() ? filename : m_productId;
    
    // Create body from surfaces
    auto body = std::make_shared<Body>();
    body->name = "Imported Geometry";
    
    for (auto& [id, surface] : m_surfaces) {
        auto face = std::make_shared<Face>();
        face->surface = surface;
        
        // Apply color if available
        auto& dirEntry = m_directoryEntries[(id - 1) / 2];
        if (dirEntry.colorNumber > 0) {
            auto colorIt = m_colors.find(dirEntry.colorNumber);
            if (colorIt != m_colors.end()) {
                face->color = colorIt->second;
            }
        }
        
        body->faces.push_back(face);
    }
    
    if (!body->faces.empty()) {
        model->bodies.push_back(body);
    }
    
    return model;
}

bool IGESImporter::parseFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        m_errorMessage = "Failed to open file: " + filename;
        return false;
    }
    
    std::vector<std::string> startLines, globalLines, directoryLines, parameterLines;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.length() < 73) {
            line.resize(80, ' ');
        }
        
        char section = line[72];
        
        switch (section) {
            case 'S':
                startLines.push_back(line.substr(0, 72));
                break;
            case 'G':
                globalLines.push_back(line.substr(0, 72));
                break;
            case 'D':
                directoryLines.push_back(line.substr(0, 72));
                break;
            case 'P':
                parameterLines.push_back(line.substr(0, 72));
                break;
            case 'T':
                // Terminate section - stop reading
                break;
        }
    }
    
    file.close();
    
    // Parse each section
    if (!parseStartSection(startLines)) return false;
    if (!parseGlobalSection(globalLines)) return false;
    if (!parseDirectorySection(directoryLines)) return false;
    if (!parseParameterSection(parameterLines)) return false;
    
    return true;
}

bool IGESImporter::parseStartSection(const std::vector<std::string>& lines)
{
    m_startSection.clear();
    for (const auto& line : lines) {
        m_startSection += line;
    }
    return true;
}

bool IGESImporter::parseGlobalSection(const std::vector<std::string>& lines)
{
    m_globalSection.clear();
    for (const auto& line : lines) {
        m_globalSection += line;
    }
    
    // Parse global parameters
    auto params = parseParameterData(m_globalSection);
    
    if (params.size() > 0) m_paramDelimiter = params[0].empty() ? ',' : params[0][0];
    if (params.size() > 1) m_recordDelimiter = params[1].empty() ? ';' : params[1][0];
    if (params.size() > 2) m_productId = parseHollerith(params[2]);
    if (params.size() > 3) m_fileName = parseHollerith(params[3]);
    if (params.size() > 12) m_modelScale = parseReal(params[12]);
    if (params.size() > 13) m_unitsFlag = parseInt(params[13]);
    if (params.size() > 19) m_maxCoordValue = parseReal(params[19]);
    
    return true;
}

bool IGESImporter::parseDirectorySection(const std::vector<std::string>& lines)
{
    // Directory entries come in pairs of lines
    for (size_t i = 0; i + 1 < lines.size(); i += 2) {
        IGESDirectoryEntry entry;
        
        // First line
        std::string line1 = lines[i];
        entry.entityType = parseInt(line1.substr(0, 8));
        entry.parameterData = parseInt(line1.substr(8, 8));
        entry.structure = parseInt(line1.substr(16, 8));
        entry.lineFontPattern = parseInt(line1.substr(24, 8));
        entry.level = parseInt(line1.substr(32, 8));
        entry.view = parseInt(line1.substr(40, 8));
        entry.transformationMatrix = parseInt(line1.substr(48, 8));
        entry.labelDisplayAssoc = parseInt(line1.substr(56, 8));
        entry.statusNumber = parseInt(line1.substr(64, 8));
        
        // Second line
        std::string line2 = lines[i + 1];
        entry.lineWeight = parseInt(line2.substr(8, 8));
        entry.colorNumber = parseInt(line2.substr(16, 8));
        entry.parameterLineCount = parseInt(line2.substr(24, 8));
        entry.formNumber = parseInt(line2.substr(32, 8));
        entry.entityLabel = line2.substr(56, 8);
        entry.entitySubscript = line2.substr(64, 8);
        
        // Trim whitespace from label
        entry.entityLabel.erase(entry.entityLabel.find_last_not_of(" \t") + 1);
        
        entry.sequenceNumber = (i / 2) + 1;
        
        m_directoryEntries.push_back(entry);
        m_stats.totalEntities++;
    }
    
    return true;
}

bool IGESImporter::parseParameterSection(const std::vector<std::string>& lines)
{
    // Combine parameter lines for each entity
    std::map<int, std::string> paramData;  // Directory entry -> accumulated parameter data
    
    for (const auto& line : lines) {
        if (line.length() < 72) continue;
        
        std::string data = line.substr(0, 64);
        int dirEntryNum = parseInt(line.substr(64, 8));
        
        // Remove trailing spaces
        data.erase(data.find_last_not_of(" \t") + 1);
        
        paramData[dirEntryNum] += data;
    }
    
    // Parse each entity's parameter data
    for (auto& [dirNum, data] : paramData) {
        IGESParameterEntry entry;
        entry.directoryEntry = dirNum;
        
        // First parameter is entity type
        auto params = parseParameterData(data);
        if (!params.empty()) {
            entry.entityType = parseInt(params[0]);
            entry.parameters = std::vector<std::string>(params.begin() + 1, params.end());
        }
        
        m_parameterEntries[dirNum] = entry;
    }
    
    return true;
}

std::vector<std::string> IGESImporter::parseParameterData(const std::string& data)
{
    std::vector<std::string> params;
    std::string current;
    bool inHollerith = false;
    int hollerithCount = 0;
    
    for (size_t i = 0; i < data.length(); i++) {
        char c = data[i];
        
        if (inHollerith) {
            current += c;
            hollerithCount--;
            if (hollerithCount <= 0) {
                inHollerith = false;
            }
        }
        else if (std::isdigit(c) && current.empty()) {
            // Possible Hollerith string start
            current += c;
        }
        else if (c == 'H' && !current.empty() && std::all_of(current.begin(), current.end(), ::isdigit)) {
            // Hollerith string
            hollerithCount = std::stoi(current);
            // HIGH FIX: Validate Hollerith count to prevent buffer over-read
            if (hollerithCount < 0 || i + static_cast<size_t>(hollerithCount) >= data.length()) {
                // Malformed Hollerith string - treat as regular token and continue
                current += c;
                continue;
            }
            current = std::to_string(hollerithCount) + "H";
            inHollerith = true;
        }
        else if (c == m_paramDelimiter || c == m_recordDelimiter) {
            params.push_back(current);
            current.clear();
            if (c == m_recordDelimiter) break;
        }
        else if (c != ' ' || !current.empty()) {
            current += c;
        }
    }
    
    if (!current.empty()) {
        params.push_back(current);
    }
    
    return params;
}

bool IGESImporter::processEntities()
{
    double unitScale = getUnitScale();
    
    for (const auto& entry : m_directoryEntries) {
        int dirNum = entry.sequenceNumber * 2 - 1;  // Directory entry number (odd)
        
        auto paramIt = m_parameterEntries.find(dirNum);
        if (paramIt == m_parameterEntries.end()) continue;
        
        switch (entry.entityType) {
            case 116:  // Point
                m_points[dirNum] = getPoint(dirNum);
                m_stats.pointsImported++;
                break;
                
            case 110:  // Line
                m_curves[dirNum] = getLine(dirNum);
                m_stats.curvesImported++;
                break;
                
            case 100:  // Circular Arc
                m_curves[dirNum] = getCircularArc(dirNum);
                m_stats.curvesImported++;
                break;
                
            case 126:  // Rational B-Spline Curve
                m_curves[dirNum] = getRationalBSplineCurve(dirNum);
                m_stats.curvesImported++;
                break;
                
            case 102:  // Composite Curve
                m_curves[dirNum] = getCompositeCurve(dirNum);
                m_stats.curvesImported++;
                break;
                
            case 108:  // Plane
                m_surfaces[dirNum] = getPlane(dirNum);
                m_stats.surfacesImported++;
                break;
                
            case 118:  // Ruled Surface
                m_surfaces[dirNum] = getRuledSurface(dirNum);
                m_stats.surfacesImported++;
                break;
                
            case 120:  // Surface of Revolution
                m_surfaces[dirNum] = getSurfaceOfRevolution(dirNum);
                m_stats.surfacesImported++;
                break;
                
            case 122:  // Tabulated Cylinder
                m_surfaces[dirNum] = getTabulatedCylinder(dirNum);
                m_stats.surfacesImported++;
                break;
                
            case 128:  // Rational B-Spline Surface
                m_surfaces[dirNum] = getRationalBSplineSurface(dirNum);
                m_stats.surfacesImported++;
                break;
                
            case 144:  // Trimmed Parametric Surface
                m_surfaces[dirNum] = getTrimmedSurface(dirNum);
                m_stats.surfacesImported++;
                break;
                
            case 124:  // Transformation Matrix
                m_transformations[dirNum] = getTransformationMatrix(dirNum);
                break;
                
            case 314:  // Color Definition
                m_colors[dirNum] = getColorDefinition(dirNum);
                break;
        }
    }
    
    return true;
}

glm::dvec3 IGESImporter::getPoint(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 3) {
        return glm::dvec3(0);
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    return glm::dvec3(
        parseReal(params[0]) * scale,
        parseReal(params[1]) * scale,
        parseReal(params[2]) * scale
    );
}

std::shared_ptr<NURBSCurve> IGESImporter::getLine(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 6) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    auto curve = std::make_shared<NURBSCurve>();
    curve->degree = 1;
    
    curve->controlPoints.push_back(glm::dvec3(
        parseReal(params[0]) * scale,
        parseReal(params[1]) * scale,
        parseReal(params[2]) * scale
    ));
    
    curve->controlPoints.push_back(glm::dvec3(
        parseReal(params[3]) * scale,
        parseReal(params[4]) * scale,
        parseReal(params[5]) * scale
    ));
    
    curve->knots = {0, 0, 1, 1};
    
    return curve;
}

std::shared_ptr<NURBSCurve> IGESImporter::getCircularArc(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 6) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    double zt = parseReal(params[0]) * scale;  // Z displacement
    double x1 = parseReal(params[1]) * scale;  // Center X
    double y1 = parseReal(params[2]) * scale;  // Center Y
    double x2 = parseReal(params[3]) * scale;  // Start X
    double y2 = parseReal(params[4]) * scale;  // Start Y
    double x3 = parseReal(params[5]) * scale;  // End X
    double y3 = parseReal(params[6]) * scale;  // End Y
    
    glm::dvec3 center(x1, y1, zt);
    glm::dvec3 start(x2, y2, zt);
    glm::dvec3 end(x3, y3, zt);
    
    double radius = glm::length(start - center);
    double startAngle = std::atan2(y2 - y1, x2 - x1);
    double endAngle = std::atan2(y3 - y1, x3 - x1);
    
    if (endAngle < startAngle) {
        endAngle += 2.0 * M_PI;
    }
    
    // Create rational quadratic arc
    auto curve = std::make_shared<NURBSCurve>();
    curve->degree = 2;
    
    // Simple 3-point arc
    double midAngle = (startAngle + endAngle) / 2.0;
    double halfAngle = (endAngle - startAngle) / 2.0;
    double w = std::cos(halfAngle);
    
    glm::dvec3 mid = center + glm::dvec3(
        radius * std::cos(midAngle) / w,
        radius * std::sin(midAngle) / w,
        0
    );
    
    curve->controlPoints = {start, mid, end};
    curve->weights = {1.0, w, 1.0};
    curve->knots = {0, 0, 0, 1, 1, 1};
    
    return curve;
}

std::shared_ptr<NURBSCurve> IGESImporter::getCompositeCurve(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.empty()) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    int numCurves = parseInt(params[0]);
    
    // Combine all referenced curves
    auto composite = std::make_shared<NURBSCurve>();
    composite->degree = 1;  // Will be updated based on component curves
    
    for (int i = 1; i <= numCurves && i < (int)params.size(); i++) {
        int curveRef = parseInt(params[i]);
        
        // Find curve in our map
        auto curveIt = m_curves.find(curveRef);
        if (curveIt != m_curves.end() && curveIt->second) {
            // Append control points
            for (const auto& cp : curveIt->second->controlPoints) {
                composite->controlPoints.push_back(cp);
            }
        }
    }
    
    return composite;
}

std::shared_ptr<NURBSCurve> IGESImporter::getRationalBSplineCurve(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 7) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    int K = parseInt(params[0]);   // Upper index of sum
    int M = parseInt(params[1]);   // Degree
    int PROP1 = parseInt(params[2]);  // Planar
    int PROP2 = parseInt(params[3]);  // Closed
    int PROP3 = parseInt(params[4]);  // Polynomial (non-rational)
    int PROP4 = parseInt(params[5]);  // Periodic
    
    auto curve = std::make_shared<NURBSCurve>();
    curve->degree = M;
    curve->isPlanar = (PROP1 == 1);
    curve->isClosed = (PROP2 == 1);
    
    int N = K + M + 1;  // Number of knots
    int idx = 6;
    
    // Read knot sequence
    for (int i = 0; i <= N && idx < (int)params.size(); i++, idx++) {
        curve->knots.push_back(parseReal(params[idx]));
    }
    
    // Read weights
    for (int i = 0; i <= K && idx < (int)params.size(); i++, idx++) {
        curve->weights.push_back(parseReal(params[idx]));
    }
    
    // Read control points
    for (int i = 0; i <= K && idx + 2 < (int)params.size(); i++) {
        curve->controlPoints.push_back(glm::dvec3(
            parseReal(params[idx++]) * scale,
            parseReal(params[idx++]) * scale,
            parseReal(params[idx++]) * scale
        ));
    }
    
    // Read parameter range
    if (idx < (int)params.size()) curve->tMin = parseReal(params[idx++]);
    if (idx < (int)params.size()) curve->tMax = parseReal(params[idx++]);
    
    return curve;
}

std::shared_ptr<NURBSSurface> IGESImporter::getPlane(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 4) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    // Form 0: A*X + B*Y + C*Z = D
    double A = parseReal(params[0]);
    double B = parseReal(params[1]);
    double C = parseReal(params[2]);
    double D = parseReal(params[3]) * scale;
    
    glm::dvec3 normal = glm::normalize(glm::dvec3(A, B, C));
    glm::dvec3 origin = normal * D;
    
    // Create orthogonal basis
    glm::dvec3 xAxis = std::abs(normal.x) < 0.9 
        ? glm::normalize(glm::cross(normal, glm::dvec3(1, 0, 0)))
        : glm::normalize(glm::cross(normal, glm::dvec3(0, 1, 0)));
    glm::dvec3 yAxis = glm::cross(normal, xAxis);
    
    auto surface = std::make_shared<NURBSSurface>();
    surface->degreeU = 1;
    surface->degreeV = 1;
    
    double size = 1000.0;
    surface->controlPoints = {
        {origin - size * xAxis - size * yAxis, origin + size * xAxis - size * yAxis},
        {origin - size * xAxis + size * yAxis, origin + size * xAxis + size * yAxis}
    };
    
    surface->knotsU = {0, 0, 1, 1};
    surface->knotsV = {0, 0, 1, 1};
    
    return surface;
}

std::shared_ptr<NURBSSurface> IGESImporter::getRuledSurface(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 3) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    
    int curve1Ref = parseInt(params[0]);
    int curve2Ref = parseInt(params[1]);
    int dirFlag = parseInt(params[2]);
    int devFlag = params.size() > 3 ? parseInt(params[3]) : 0;
    
    // Get the two generator curves
    std::shared_ptr<NURBSCurve> curve1, curve2;
    
    auto it1 = m_curves.find(curve1Ref);
    auto it2 = m_curves.find(curve2Ref);
    
    if (it1 == m_curves.end() || it2 == m_curves.end() ||
        !it1->second || !it2->second) {
        return nullptr;
    }
    
    curve1 = it1->second;
    curve2 = it2->second;
    
    // Create ruled surface
    auto surface = std::make_shared<NURBSSurface>();
    surface->degreeU = std::max(curve1->degree, curve2->degree);
    surface->degreeV = 1;
    
    // Use control points from both curves
    size_t numU = std::max(curve1->controlPoints.size(), curve2->controlPoints.size());
    
    for (size_t i = 0; i < numU; i++) {
        glm::dvec3 p1 = i < curve1->controlPoints.size() 
            ? curve1->controlPoints[i] 
            : curve1->controlPoints.back();
        glm::dvec3 p2 = i < curve2->controlPoints.size() 
            ? curve2->controlPoints[i] 
            : curve2->controlPoints.back();
        
        surface->controlPoints.push_back({p1, p2});
    }
    
    surface->knotsU = curve1->knots;
    surface->knotsV = {0, 0, 1, 1};
    
    return surface;
}

std::shared_ptr<NURBSSurface> IGESImporter::getSurfaceOfRevolution(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 4) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    
    int lineRef = parseInt(params[0]);     // Axis line
    int curveRef = parseInt(params[1]);    // Generatrix curve
    double startAngle = parseReal(params[2]);
    double endAngle = parseReal(params[3]);
    
    // Get generatrix curve
    auto curveIt = m_curves.find(curveRef);
    if (curveIt == m_curves.end() || !curveIt->second) {
        return nullptr;
    }
    
    auto curve = curveIt->second;
    
    // Get axis (assume Z-axis for simplicity)
    glm::dvec3 axisPoint(0, 0, 0);
    glm::dvec3 axisDir(0, 0, 1);
    
    // Create surface of revolution
    auto surface = std::make_shared<NURBSSurface>();
    surface->degreeU = curve->degree;
    surface->degreeV = 2;  // Rational quadratic for circular sections
    
    // Number of sections (quarters)
    int numSections = std::ceil((endAngle - startAngle) / (M_PI / 2.0));
    numSections = std::max(1, numSections);
    
    double angleStep = (endAngle - startAngle) / numSections;
    double w = std::cos(angleStep / 2.0);
    
    // For each control point on the generatrix
    for (const auto& cp : curve->controlPoints) {
        double radius = std::sqrt(cp.x * cp.x + cp.y * cp.y);
        double height = cp.z;
        
        std::vector<glm::dvec3> row;
        
        for (int i = 0; i <= numSections; i++) {
            double angle = startAngle + i * angleStep;
            
            // Main point
            row.push_back(glm::dvec3(
                radius * std::cos(angle),
                radius * std::sin(angle),
                height
            ));
            
            // Control point for arc (if not last)
            if (i < numSections) {
                double midAngle = angle + angleStep / 2.0;
                row.push_back(glm::dvec3(
                    radius * std::cos(midAngle) / w,
                    radius * std::sin(midAngle) / w,
                    height
                ));
            }
        }
        
        surface->controlPoints.push_back(row);
    }
    
    // Build knots
    surface->knotsU = curve->knots;
    
    surface->knotsV = {0, 0, 0};
    for (int i = 1; i < numSections; i++) {
        double t = (double)i / numSections;
        surface->knotsV.push_back(t);
        surface->knotsV.push_back(t);
    }
    surface->knotsV.push_back(1);
    surface->knotsV.push_back(1);
    surface->knotsV.push_back(1);
    
    return surface;
}

std::shared_ptr<NURBSSurface> IGESImporter::getTabulatedCylinder(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 4) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    int curveRef = parseInt(params[0]);
    double lx = parseReal(params[1]) * scale;
    double ly = parseReal(params[2]) * scale;
    double lz = parseReal(params[3]) * scale;
    
    glm::dvec3 direction(lx, ly, lz);
    
    auto curveIt = m_curves.find(curveRef);
    if (curveIt == m_curves.end() || !curveIt->second) {
        return nullptr;
    }
    
    auto curve = curveIt->second;
    
    auto surface = std::make_shared<NURBSSurface>();
    surface->degreeU = curve->degree;
    surface->degreeV = 1;
    
    // Extrude each control point along direction
    for (const auto& cp : curve->controlPoints) {
        surface->controlPoints.push_back({cp, cp + direction});
    }
    
    surface->knotsU = curve->knots;
    surface->knotsV = {0, 0, 1, 1};
    
    return surface;
}

std::shared_ptr<NURBSSurface> IGESImporter::getRationalBSplineSurface(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 10) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    int K1 = parseInt(params[0]);    // Upper index U
    int K2 = parseInt(params[1]);    // Upper index V
    int M1 = parseInt(params[2]);    // Degree U
    int M2 = parseInt(params[3]);    // Degree V
    int PROP1 = parseInt(params[4]); // Closed in U
    int PROP2 = parseInt(params[5]); // Closed in V
    int PROP3 = parseInt(params[6]); // Polynomial
    int PROP4 = parseInt(params[7]); // Periodic in U
    int PROP5 = parseInt(params[8]); // Periodic in V
    
    auto surface = std::make_shared<NURBSSurface>();
    surface->degreeU = M1;
    surface->degreeV = M2;
    
    int N1 = K1 + M1 + 1;  // Number of U knots
    int N2 = K2 + M2 + 1;  // Number of V knots
    int idx = 9;
    
    // Read U knots
    for (int i = 0; i <= N1 && idx < (int)params.size(); i++, idx++) {
        surface->knotsU.push_back(parseReal(params[idx]));
    }
    
    // Read V knots
    for (int i = 0; i <= N2 && idx < (int)params.size(); i++, idx++) {
        surface->knotsV.push_back(parseReal(params[idx]));
    }
    
    // Read weights
    surface->weights.resize(K1 + 1);
    for (int i = 0; i <= K1; i++) {
        surface->weights[i].resize(K2 + 1);
        for (int j = 0; j <= K2 && idx < (int)params.size(); j++, idx++) {
            surface->weights[i][j] = parseReal(params[idx]);
        }
    }
    
    // Read control points
    surface->controlPoints.resize(K1 + 1);
    for (int i = 0; i <= K1; i++) {
        surface->controlPoints[i].resize(K2 + 1);
        for (int j = 0; j <= K2 && idx + 2 < (int)params.size(); j++) {
            surface->controlPoints[i][j] = glm::dvec3(
                parseReal(params[idx++]) * scale,
                parseReal(params[idx++]) * scale,
                parseReal(params[idx++]) * scale
            );
        }
    }
    
    // Read parameter range
    if (idx < (int)params.size()) surface->uMin = parseReal(params[idx++]);
    if (idx < (int)params.size()) surface->uMax = parseReal(params[idx++]);
    if (idx < (int)params.size()) surface->vMin = parseReal(params[idx++]);
    if (idx < (int)params.size()) surface->vMax = parseReal(params[idx++]);
    
    return surface;
}

std::shared_ptr<NURBSSurface> IGESImporter::getTrimmedSurface(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 3) {
        return nullptr;
    }
    
    auto& params = paramIt->second.parameters;
    
    int surfaceRef = parseInt(params[0]);
    int n1 = parseInt(params[1]);  // Outer boundary flag
    int n2 = parseInt(params[2]);  // Number of inner boundaries
    
    // Get the base surface
    auto surfIt = m_surfaces.find(surfaceRef);
    if (surfIt != m_surfaces.end()) {
        // Return copy of base surface (trimming handled separately)
        auto trimmed = std::make_shared<NURBSSurface>(*surfIt->second);
        return trimmed;
    }
    
    return nullptr;
}

glm::dmat4 IGESImporter::getTransformationMatrix(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 12) {
        return glm::dmat4(1.0);
    }
    
    auto& params = paramIt->second.parameters;
    double scale = getUnitScale();
    
    // IGES transformation matrix: R11, R12, R13, T1, R21, R22, R23, T2, R31, R32, R33, T3
    glm::dmat4 matrix(1.0);
    
    matrix[0][0] = parseReal(params[0]);
    matrix[1][0] = parseReal(params[1]);
    matrix[2][0] = parseReal(params[2]);
    matrix[3][0] = parseReal(params[3]) * scale;
    
    matrix[0][1] = parseReal(params[4]);
    matrix[1][1] = parseReal(params[5]);
    matrix[2][1] = parseReal(params[6]);
    matrix[3][1] = parseReal(params[7]) * scale;
    
    matrix[0][2] = parseReal(params[8]);
    matrix[1][2] = parseReal(params[9]);
    matrix[2][2] = parseReal(params[10]);
    matrix[3][2] = parseReal(params[11]) * scale;
    
    return matrix;
}

glm::vec3 IGESImporter::getColorDefinition(int directoryEntry)
{
    auto paramIt = m_parameterEntries.find(directoryEntry);
    if (paramIt == m_parameterEntries.end() || paramIt->second.parameters.size() < 3) {
        return glm::vec3(0.7f);
    }
    
    auto& params = paramIt->second.parameters;
    
    // Colors are 0-100 range
    return glm::vec3(
        parseReal(params[0]) / 100.0f,
        parseReal(params[1]) / 100.0f,
        parseReal(params[2]) / 100.0f
    );
}

double IGESImporter::parseReal(const std::string& str)
{
    std::string s = str;
    
    // Remove leading/trailing whitespace
    s.erase(0, s.find_first_not_of(" \t"));
    s.erase(s.find_last_not_of(" \t") + 1);
    
    // Handle IGES D exponent notation
    std::replace(s.begin(), s.end(), 'D', 'E');
    std::replace(s.begin(), s.end(), 'd', 'e');
    
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

int IGESImporter::parseInt(const std::string& str)
{
    std::string s = str;
    s.erase(0, s.find_first_not_of(" \t"));
    s.erase(s.find_last_not_of(" \t") + 1);
    
    try {
        return std::stoi(s);
    } catch (...) {
        return 0;
    }
}

std::string IGESImporter::parseHollerith(const std::string& str)
{
    // Format: nHstring where n is string length
    size_t hPos = str.find('H');
    if (hPos == std::string::npos) {
        return str;
    }
    
    int length = std::stoi(str.substr(0, hPos));
    return str.substr(hPos + 1, length);
}

double IGESImporter::getUnitScale() const
{
    // Convert from file units to mm (internal unit)
    switch (m_unitsFlag) {
        case 1:  return 25.4;       // Inches to mm
        case 2:  return 1.0;        // MM
        case 3:  return 1.0;        // See flag value (default mm)
        case 4:  return 304.8;      // Feet to mm
        case 5:  return 1609344.0;  // Miles to mm
        case 6:  return 1000.0;     // Meters to mm
        case 7:  return 1000000.0;  // Kilometers to mm
        case 8:  return 0.0254;     // Mils to mm
        case 9:  return 0.001;      // Microns to mm
        case 10: return 10.0;       // Centimeters to mm
        case 11: return 0.000001;   // Microinches to mm
        default: return 1.0;
    }
}

void IGESImporter::addWarning(const std::string& msg)
{
    m_stats.warnings.push_back(msg);
}

} // namespace dc
