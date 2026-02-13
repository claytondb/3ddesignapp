#include "NativeFormat.h"
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cstring>
#include <algorithm>

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
};

class SketchElement {
public:
    enum class Type { Line, Arc, Circle, Spline };
    Type type = Type::Line;
    std::vector<glm::dvec2> points;
    std::vector<double> parameters;
    bool isConstruction = false;
};

class Sketch {
public:
    std::string name = "Sketch";
    glm::dvec3 origin{0, 0, 0};
    glm::dvec3 normal{0, 0, 1};
    glm::dvec3 xAxis{1, 0, 0};
    std::vector<std::shared_ptr<SketchElement>> elements;
    bool isVisible = true;
};

class Face {
public:
    std::shared_ptr<NURBSSurface> surface;
    MeshData mesh;
    glm::vec3 color{0.7f, 0.7f, 0.8f};
};

class Body {
public:
    std::string name = "Body";
    std::string id;
    std::vector<std::shared_ptr<Face>> faces;
    MeshData combinedMesh;
    glm::vec3 color{0.7f, 0.7f, 0.8f};
    bool isVisible = true;
};

class Project {
public:
    ProjectSettings settings;
    std::vector<std::shared_ptr<Body>> bodies;
    std::vector<std::shared_ptr<Sketch>> sketches;
    std::string filePath;
};

NativeFormat::NativeFormat()
{
}

NativeFormat::~NativeFormat()
{
}

bool NativeFormat::saveProject(const Project& project, const std::string& filename)
{
    m_errorMessage.clear();
    
    try {
        std::vector<SimpleArchive::Entry> entries;
        
        // Create manifest JSON
        std::string manifest = createManifestJSON(project);
        SimpleArchive::Entry manifestEntry;
        manifestEntry.name = "manifest.json";
        manifestEntry.data = std::vector<uint8_t>(manifest.begin(), manifest.end());
        entries.push_back(manifestEntry);
        
        // Serialize bodies
        for (size_t i = 0; i < project.bodies.size(); i++) {
            const auto& body = project.bodies[i];
            
            // Mesh data
            auto meshData = serializeMeshData(body->combinedMesh);
            if (!meshData.empty()) {
                SimpleArchive::Entry meshEntry;
                meshEntry.name = "meshes/body_" + std::to_string(i) + ".bin";
                meshEntry.data = meshData;
                entries.push_back(meshEntry);
            }
            
            // Surface data for each face
            for (size_t j = 0; j < body->faces.size(); j++) {
                const auto& face = body->faces[j];
                if (face->surface) {
                    auto surfData = serializeSurfaceData(*face->surface);
                    SimpleArchive::Entry surfEntry;
                    surfEntry.name = "surfaces/body_" + std::to_string(i) + "_face_" + std::to_string(j) + ".bin";
                    surfEntry.data = surfData;
                    entries.push_back(surfEntry);
                }
            }
        }
        
        // Serialize sketches
        for (size_t i = 0; i < project.sketches.size(); i++) {
            auto sketchData = serializeSketchData(*project.sketches[i]);
            SimpleArchive::Entry sketchEntry;
            sketchEntry.name = "sketches/sketch_" + std::to_string(i) + ".bin";
            sketchEntry.data = sketchData;
            entries.push_back(sketchEntry);
        }
        
        // Write archive
        SimpleArchive archive;
        if (!archive.write(filename, entries)) {
            m_errorMessage = "Failed to write archive file";
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        m_errorMessage = std::string("Save error: ") + e.what();
        return false;
    }
}

std::shared_ptr<Project> NativeFormat::loadProject(const std::string& filename)
{
    m_errorMessage.clear();
    
    try {
        // Read archive
        SimpleArchive archive;
        std::vector<SimpleArchive::Entry> entries;
        
        if (!archive.read(filename, entries)) {
            m_errorMessage = "Failed to read archive file";
            return nullptr;
        }
        
        auto project = std::make_shared<Project>();
        project->filePath = filename;
        
        // Find and parse manifest
        for (const auto& entry : entries) {
            if (entry.name == "manifest.json") {
                std::string json(entry.data.begin(), entry.data.end());
                if (!parseManifestJSON(json, *project)) {
                    return nullptr;
                }
                break;
            }
        }
        
        // Load mesh data
        std::map<std::string, std::vector<uint8_t>> dataMap;
        for (const auto& entry : entries) {
            dataMap[entry.name] = entry.data;
        }
        
        // Load body meshes
        for (size_t i = 0; i < project->bodies.size(); i++) {
            std::string meshName = "meshes/body_" + std::to_string(i) + ".bin";
            auto it = dataMap.find(meshName);
            if (it != dataMap.end()) {
                project->bodies[i]->combinedMesh = deserializeMeshData(it->second);
            }
            
            // Load face surfaces
            for (size_t j = 0; j < project->bodies[i]->faces.size(); j++) {
                std::string surfName = "surfaces/body_" + std::to_string(i) + "_face_" + std::to_string(j) + ".bin";
                auto surfIt = dataMap.find(surfName);
                if (surfIt != dataMap.end()) {
                    project->bodies[i]->faces[j]->surface = deserializeSurfaceData(surfIt->second);
                }
            }
        }
        
        // Load sketches
        for (size_t i = 0; i < project->sketches.size(); i++) {
            std::string sketchName = "sketches/sketch_" + std::to_string(i) + ".bin";
            auto it = dataMap.find(sketchName);
            if (it != dataMap.end()) {
                project->sketches[i] = deserializeSketchData(it->second);
            }
        }
        
        return project;
    }
    catch (const std::exception& e) {
        m_errorMessage = std::string("Load error: ") + e.what();
        return nullptr;
    }
}

bool NativeFormat::isValidDCAFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    
    return magic == SimpleArchive::ARCHIVE_MAGIC || magic == DCA_MAGIC_NUMBER;
}

NativeFormat::FileInfo NativeFormat::getFileInfo(const std::string& filename)
{
    FileInfo info;
    
    try {
        SimpleArchive archive;
        std::vector<SimpleArchive::Entry> entries;
        
        if (!archive.read(filename, entries)) {
            return info;
        }
        
        // Find manifest
        for (const auto& entry : entries) {
            if (entry.name == "manifest.json") {
                // Simple JSON parsing for basic info
                std::string json(entry.data.begin(), entry.data.end());
                
                // Extract name
                size_t pos = json.find("\"name\"");
                if (pos != std::string::npos) {
                    pos = json.find(":", pos);
                    size_t start = json.find("\"", pos) + 1;
                    size_t end = json.find("\"", start);
                    info.name = json.substr(start, end - start);
                }
                
                // Extract author
                pos = json.find("\"author\"");
                if (pos != std::string::npos) {
                    pos = json.find(":", pos);
                    size_t start = json.find("\"", pos) + 1;
                    size_t end = json.find("\"", start);
                    info.author = json.substr(start, end - start);
                }
                
                break;
            }
        }
        
        // Count bodies and sketches
        for (const auto& entry : entries) {
            if (entry.name.find("meshes/body_") == 0) info.bodyCount++;
            if (entry.name.find("sketches/sketch_") == 0) info.sketchCount++;
        }
    }
    catch (...) {
        // Return partial info on error
    }
    
    return info;
}

std::string NativeFormat::createManifestJSON(const Project& project)
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << DCA_FORMAT_VERSION << ",\n";
    ss << "  \"magic\": \"DCA3\",\n";
    ss << "  \"settings\": {\n";
    ss << "    \"name\": \"" << project.settings.name << "\",\n";
    ss << "    \"author\": \"" << project.settings.author << "\",\n";
    ss << "    \"description\": \"" << project.settings.description << "\",\n";
    ss << "    \"units\": \"" << project.settings.units << "\",\n";
    ss << "    \"gridSize\": " << project.settings.gridSize << ",\n";
    ss << "    \"snapTolerance\": " << project.settings.snapTolerance << ",\n";
    ss << "    \"backgroundColor\": [" 
       << project.settings.backgroundColor.r << ", "
       << project.settings.backgroundColor.g << ", "
       << project.settings.backgroundColor.b << "],\n";
    ss << "    \"showGrid\": " << (project.settings.showGrid ? "true" : "false") << ",\n";
    ss << "    \"showAxes\": " << (project.settings.showAxes ? "true" : "false") << ",\n";
    ss << "    \"showOrigin\": " << (project.settings.showOrigin ? "true" : "false") << ",\n";
    ss << "    \"createdDate\": \"" << project.settings.createdDate << "\",\n";
    ss << "    \"modifiedDate\": \"" << getCurrentTimestamp() << "\",\n";
    ss << "    \"appVersion\": \"" << project.settings.appVersion << "\"\n";
    ss << "  },\n";
    
    // Bodies
    ss << "  \"bodies\": [\n";
    for (size_t i = 0; i < project.bodies.size(); i++) {
        const auto& body = project.bodies[i];
        ss << "    {\n";
        ss << "      \"name\": \"" << body->name << "\",\n";
        ss << "      \"id\": \"" << body->id << "\",\n";
        ss << "      \"color\": [" << body->color.r << ", " << body->color.g << ", " << body->color.b << "],\n";
        ss << "      \"visible\": " << (body->isVisible ? "true" : "false") << ",\n";
        ss << "      \"faceCount\": " << body->faces.size() << "\n";
        ss << "    }" << (i < project.bodies.size() - 1 ? "," : "") << "\n";
    }
    ss << "  ],\n";
    
    // Sketches
    ss << "  \"sketches\": [\n";
    for (size_t i = 0; i < project.sketches.size(); i++) {
        const auto& sketch = project.sketches[i];
        ss << "    {\n";
        ss << "      \"name\": \"" << sketch->name << "\",\n";
        ss << "      \"origin\": [" << sketch->origin.x << ", " << sketch->origin.y << ", " << sketch->origin.z << "],\n";
        ss << "      \"normal\": [" << sketch->normal.x << ", " << sketch->normal.y << ", " << sketch->normal.z << "],\n";
        ss << "      \"xAxis\": [" << sketch->xAxis.x << ", " << sketch->xAxis.y << ", " << sketch->xAxis.z << "],\n";
        ss << "      \"visible\": " << (sketch->isVisible ? "true" : "false") << ",\n";
        ss << "      \"elementCount\": " << sketch->elements.size() << "\n";
        ss << "    }" << (i < project.sketches.size() - 1 ? "," : "") << "\n";
    }
    ss << "  ]\n";
    
    ss << "}\n";
    
    return ss.str();
}

bool NativeFormat::parseManifestJSON(const std::string& json, Project& project)
{
    // Simple JSON parser (in production, use a proper library like nlohmann/json)
    
    // Parse settings
    auto extractString = [&json](const std::string& key) -> std::string {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = json.find(":", pos);
        size_t start = json.find("\"", pos) + 1;
        size_t end = json.find("\"", start);
        return json.substr(start, end - start);
    };
    
    auto extractNumber = [&json](const std::string& key) -> double {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return 0;
        pos = json.find(":", pos);
        size_t start = pos + 1;
        while (start < json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
        size_t end = start;
        while (end < json.length() && (std::isdigit(json[end]) || json[end] == '.' || json[end] == '-')) end++;
        return std::stod(json.substr(start, end - start));
    };
    
    auto extractBool = [&json](const std::string& key) -> bool {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return false;
        return json.find("true", pos) < json.find(",", pos);
    };
    
    project.settings.name = extractString("name");
    project.settings.author = extractString("author");
    project.settings.description = extractString("description");
    project.settings.units = extractString("units");
    project.settings.gridSize = extractNumber("gridSize");
    project.settings.snapTolerance = extractNumber("snapTolerance");
    project.settings.showGrid = extractBool("showGrid");
    project.settings.showAxes = extractBool("showAxes");
    project.settings.showOrigin = extractBool("showOrigin");
    project.settings.createdDate = extractString("createdDate");
    project.settings.modifiedDate = extractString("modifiedDate");
    project.settings.appVersion = extractString("appVersion");
    
    // Count bodies and sketches (simplified - count array elements)
    size_t bodiesStart = json.find("\"bodies\"");
    size_t bodiesEnd = json.find("]", bodiesStart);
    size_t bodyCount = 0;
    for (size_t i = bodiesStart; i < bodiesEnd; i++) {
        if (json[i] == '{') bodyCount++;
    }
    
    size_t sketchesStart = json.find("\"sketches\"");
    size_t sketchesEnd = json.find("]", sketchesStart);
    size_t sketchCount = 0;
    for (size_t i = sketchesStart; i < sketchesEnd; i++) {
        if (json[i] == '{') sketchCount++;
    }
    
    // Create placeholder bodies and sketches
    for (size_t i = 0; i < bodyCount; i++) {
        project.bodies.push_back(std::make_shared<Body>());
    }
    
    for (size_t i = 0; i < sketchCount; i++) {
        project.sketches.push_back(std::make_shared<Sketch>());
    }
    
    return true;
}

std::vector<uint8_t> NativeFormat::serializeMeshData(const MeshData& mesh)
{
    std::vector<uint8_t> buffer;
    
    // Chunk header
    writeUint32(buffer, static_cast<uint32_t>(DCAChunkType::MESH_DATA));
    
    // Vertex count
    writeUint32(buffer, mesh.vertices.size());
    
    // Write vertices
    for (const auto& v : mesh.vertices) {
        writeVec3(buffer, v);
    }
    
    // Write normals
    writeUint32(buffer, mesh.normals.size());
    for (const auto& n : mesh.normals) {
        writeVec3(buffer, n);
    }
    
    // Write texture coordinates
    writeUint32(buffer, mesh.texCoords.size());
    for (const auto& tc : mesh.texCoords) {
        writeFloat(buffer, tc.x);
        writeFloat(buffer, tc.y);
    }
    
    // Write indices
    writeUint32(buffer, mesh.indices.size());
    for (uint32_t idx : mesh.indices) {
        writeUint32(buffer, idx);
    }
    
    // Bounding box
    writeVec3(buffer, mesh.boundingBoxMin);
    writeVec3(buffer, mesh.boundingBoxMax);
    
    return buffer;
}

MeshData NativeFormat::deserializeMeshData(const std::vector<uint8_t>& data)
{
    MeshData mesh;
    size_t offset = 0;
    
    // Skip chunk header
    uint32_t chunkType = readUint32(data, offset);
    
    // Read vertices
    uint32_t vertexCount = readUint32(data, offset);
    mesh.vertices.resize(vertexCount);
    for (uint32_t i = 0; i < vertexCount; i++) {
        mesh.vertices[i] = readVec3(data, offset);
    }
    
    // Read normals
    uint32_t normalCount = readUint32(data, offset);
    mesh.normals.resize(normalCount);
    for (uint32_t i = 0; i < normalCount; i++) {
        mesh.normals[i] = readVec3(data, offset);
    }
    
    // Read texture coordinates
    uint32_t texCoordCount = readUint32(data, offset);
    mesh.texCoords.resize(texCoordCount);
    for (uint32_t i = 0; i < texCoordCount; i++) {
        mesh.texCoords[i].x = readFloat(data, offset);
        mesh.texCoords[i].y = readFloat(data, offset);
    }
    
    // Read indices
    uint32_t indexCount = readUint32(data, offset);
    mesh.indices.resize(indexCount);
    for (uint32_t i = 0; i < indexCount; i++) {
        mesh.indices[i] = readUint32(data, offset);
    }
    
    // Bounding box
    mesh.boundingBoxMin = readVec3(data, offset);
    mesh.boundingBoxMax = readVec3(data, offset);
    
    return mesh;
}

std::vector<uint8_t> NativeFormat::serializeSurfaceData(const NURBSSurface& surface)
{
    std::vector<uint8_t> buffer;
    
    writeUint32(buffer, static_cast<uint32_t>(DCAChunkType::SURFACE_DATA));
    
    // Degrees
    writeUint32(buffer, surface.degreeU);
    writeUint32(buffer, surface.degreeV);
    
    // Control point grid dimensions
    uint32_t numU = surface.controlPoints.size();
    uint32_t numV = numU > 0 ? surface.controlPoints[0].size() : 0;
    writeUint32(buffer, numU);
    writeUint32(buffer, numV);
    
    // Control points
    for (const auto& row : surface.controlPoints) {
        for (const auto& cp : row) {
            writeDVec3(buffer, cp);
        }
    }
    
    // Weights
    bool hasWeights = !surface.weights.empty() && !surface.weights[0].empty();
    writeUint32(buffer, hasWeights ? 1 : 0);
    if (hasWeights) {
        for (const auto& row : surface.weights) {
            for (double w : row) {
                writeDouble(buffer, w);
            }
        }
    }
    
    // Knots U
    writeUint32(buffer, surface.knotsU.size());
    for (double k : surface.knotsU) {
        writeDouble(buffer, k);
    }
    
    // Knots V
    writeUint32(buffer, surface.knotsV.size());
    for (double k : surface.knotsV) {
        writeDouble(buffer, k);
    }
    
    // Parameter range
    writeDouble(buffer, surface.uMin);
    writeDouble(buffer, surface.uMax);
    writeDouble(buffer, surface.vMin);
    writeDouble(buffer, surface.vMax);
    
    return buffer;
}

std::shared_ptr<NURBSSurface> NativeFormat::deserializeSurfaceData(const std::vector<uint8_t>& data)
{
    auto surface = std::make_shared<NURBSSurface>();
    size_t offset = 0;
    
    // Skip chunk header
    readUint32(data, offset);
    
    surface->degreeU = readUint32(data, offset);
    surface->degreeV = readUint32(data, offset);
    
    uint32_t numU = readUint32(data, offset);
    uint32_t numV = readUint32(data, offset);
    
    // Control points
    surface->controlPoints.resize(numU);
    for (uint32_t i = 0; i < numU; i++) {
        surface->controlPoints[i].resize(numV);
        for (uint32_t j = 0; j < numV; j++) {
            surface->controlPoints[i][j] = readDVec3(data, offset);
        }
    }
    
    // Weights
    uint32_t hasWeights = readUint32(data, offset);
    if (hasWeights) {
        surface->weights.resize(numU);
        for (uint32_t i = 0; i < numU; i++) {
            surface->weights[i].resize(numV);
            for (uint32_t j = 0; j < numV; j++) {
                surface->weights[i][j] = readDouble(data, offset);
            }
        }
    }
    
    // Knots U
    uint32_t numKnotsU = readUint32(data, offset);
    surface->knotsU.resize(numKnotsU);
    for (uint32_t i = 0; i < numKnotsU; i++) {
        surface->knotsU[i] = readDouble(data, offset);
    }
    
    // Knots V
    uint32_t numKnotsV = readUint32(data, offset);
    surface->knotsV.resize(numKnotsV);
    for (uint32_t i = 0; i < numKnotsV; i++) {
        surface->knotsV[i] = readDouble(data, offset);
    }
    
    // Parameter range
    surface->uMin = readDouble(data, offset);
    surface->uMax = readDouble(data, offset);
    surface->vMin = readDouble(data, offset);
    surface->vMax = readDouble(data, offset);
    
    return surface;
}

std::vector<uint8_t> NativeFormat::serializeCurveData(const NURBSCurve& curve)
{
    std::vector<uint8_t> buffer;
    
    writeUint32(buffer, static_cast<uint32_t>(DCAChunkType::CURVE_DATA));
    
    writeUint32(buffer, curve.degree);
    
    // Control points
    writeUint32(buffer, curve.controlPoints.size());
    for (const auto& cp : curve.controlPoints) {
        writeDVec3(buffer, cp);
    }
    
    // Weights
    bool hasWeights = !curve.weights.empty();
    writeUint32(buffer, hasWeights ? 1 : 0);
    if (hasWeights) {
        for (double w : curve.weights) {
            writeDouble(buffer, w);
        }
    }
    
    // Knots
    writeUint32(buffer, curve.knots.size());
    for (double k : curve.knots) {
        writeDouble(buffer, k);
    }
    
    // Parameter range
    writeDouble(buffer, curve.tMin);
    writeDouble(buffer, curve.tMax);
    
    return buffer;
}

std::shared_ptr<NURBSCurve> NativeFormat::deserializeCurveData(const std::vector<uint8_t>& data)
{
    auto curve = std::make_shared<NURBSCurve>();
    size_t offset = 0;
    
    readUint32(data, offset);  // Skip chunk header
    
    curve->degree = readUint32(data, offset);
    
    uint32_t numCP = readUint32(data, offset);
    curve->controlPoints.resize(numCP);
    for (uint32_t i = 0; i < numCP; i++) {
        curve->controlPoints[i] = readDVec3(data, offset);
    }
    
    uint32_t hasWeights = readUint32(data, offset);
    if (hasWeights) {
        curve->weights.resize(numCP);
        for (uint32_t i = 0; i < numCP; i++) {
            curve->weights[i] = readDouble(data, offset);
        }
    }
    
    uint32_t numKnots = readUint32(data, offset);
    curve->knots.resize(numKnots);
    for (uint32_t i = 0; i < numKnots; i++) {
        curve->knots[i] = readDouble(data, offset);
    }
    
    curve->tMin = readDouble(data, offset);
    curve->tMax = readDouble(data, offset);
    
    return curve;
}

std::vector<uint8_t> NativeFormat::serializeSketchData(const Sketch& sketch)
{
    std::vector<uint8_t> buffer;
    
    writeUint32(buffer, static_cast<uint32_t>(DCAChunkType::SKETCH_DATA));
    
    writeString(buffer, sketch.name);
    writeDVec3(buffer, sketch.origin);
    writeDVec3(buffer, sketch.normal);
    writeDVec3(buffer, sketch.xAxis);
    writeUint32(buffer, sketch.isVisible ? 1 : 0);
    
    // Elements
    writeUint32(buffer, sketch.elements.size());
    for (const auto& elem : sketch.elements) {
        writeUint32(buffer, static_cast<uint32_t>(elem->type));
        writeUint32(buffer, elem->isConstruction ? 1 : 0);
        
        // Points
        writeUint32(buffer, elem->points.size());
        for (const auto& p : elem->points) {
            writeDouble(buffer, p.x);
            writeDouble(buffer, p.y);
        }
        
        // Parameters
        writeUint32(buffer, elem->parameters.size());
        for (double param : elem->parameters) {
            writeDouble(buffer, param);
        }
    }
    
    return buffer;
}

std::shared_ptr<Sketch> NativeFormat::deserializeSketchData(const std::vector<uint8_t>& data)
{
    auto sketch = std::make_shared<Sketch>();
    size_t offset = 0;
    
    readUint32(data, offset);  // Skip chunk header
    
    sketch->name = readString(data, offset);
    sketch->origin = readDVec3(data, offset);
    sketch->normal = readDVec3(data, offset);
    sketch->xAxis = readDVec3(data, offset);
    sketch->isVisible = readUint32(data, offset) != 0;
    
    uint32_t numElements = readUint32(data, offset);
    for (uint32_t i = 0; i < numElements; i++) {
        auto elem = std::make_shared<SketchElement>();
        elem->type = static_cast<SketchElement::Type>(readUint32(data, offset));
        elem->isConstruction = readUint32(data, offset) != 0;
        
        uint32_t numPoints = readUint32(data, offset);
        elem->points.resize(numPoints);
        for (uint32_t j = 0; j < numPoints; j++) {
            elem->points[j].x = readDouble(data, offset);
            elem->points[j].y = readDouble(data, offset);
        }
        
        uint32_t numParams = readUint32(data, offset);
        elem->parameters.resize(numParams);
        for (uint32_t j = 0; j < numParams; j++) {
            elem->parameters[j] = readDouble(data, offset);
        }
        
        sketch->elements.push_back(elem);
    }
    
    return sketch;
}

// Binary write helpers
void NativeFormat::writeUint32(std::vector<uint8_t>& buffer, uint32_t value)
{
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 24) & 0xFF);
}

void NativeFormat::writeFloat(std::vector<uint8_t>& buffer, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    writeUint32(buffer, bits);
}

void NativeFormat::writeDouble(std::vector<uint8_t>& buffer, double value)
{
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    buffer.push_back(bits & 0xFF);
    buffer.push_back((bits >> 8) & 0xFF);
    buffer.push_back((bits >> 16) & 0xFF);
    buffer.push_back((bits >> 24) & 0xFF);
    buffer.push_back((bits >> 32) & 0xFF);
    buffer.push_back((bits >> 40) & 0xFF);
    buffer.push_back((bits >> 48) & 0xFF);
    buffer.push_back((bits >> 56) & 0xFF);
}

void NativeFormat::writeVec3(std::vector<uint8_t>& buffer, const glm::vec3& v)
{
    writeFloat(buffer, v.x);
    writeFloat(buffer, v.y);
    writeFloat(buffer, v.z);
}

void NativeFormat::writeDVec3(std::vector<uint8_t>& buffer, const glm::dvec3& v)
{
    writeDouble(buffer, v.x);
    writeDouble(buffer, v.y);
    writeDouble(buffer, v.z);
}

void NativeFormat::writeString(std::vector<uint8_t>& buffer, const std::string& str)
{
    writeUint32(buffer, str.length());
    for (char c : str) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

// Binary read helpers
uint32_t NativeFormat::readUint32(const std::vector<uint8_t>& buffer, size_t& offset)
{
    // CRITICAL FIX: Throw exception on buffer underflow instead of silently returning 0
    if (offset + 4 > buffer.size()) {
        throw std::runtime_error("Buffer underflow in readUint32: offset " + 
            std::to_string(offset) + " + 4 > size " + std::to_string(buffer.size()));
    }
    uint32_t value = buffer[offset] |
                     (buffer[offset + 1] << 8) |
                     (buffer[offset + 2] << 16) |
                     (buffer[offset + 3] << 24);
    offset += 4;
    return value;
}

float NativeFormat::readFloat(const std::vector<uint8_t>& buffer, size_t& offset)
{
    uint32_t bits = readUint32(buffer, offset);
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

double NativeFormat::readDouble(const std::vector<uint8_t>& buffer, size_t& offset)
{
    // CRITICAL FIX: Throw exception on buffer underflow instead of silently returning 0
    if (offset + 8 > buffer.size()) {
        throw std::runtime_error("Buffer underflow in readDouble: offset " + 
            std::to_string(offset) + " + 8 > size " + std::to_string(buffer.size()));
    }
    uint64_t bits = static_cast<uint64_t>(buffer[offset]) |
                    (static_cast<uint64_t>(buffer[offset + 1]) << 8) |
                    (static_cast<uint64_t>(buffer[offset + 2]) << 16) |
                    (static_cast<uint64_t>(buffer[offset + 3]) << 24) |
                    (static_cast<uint64_t>(buffer[offset + 4]) << 32) |
                    (static_cast<uint64_t>(buffer[offset + 5]) << 40) |
                    (static_cast<uint64_t>(buffer[offset + 6]) << 48) |
                    (static_cast<uint64_t>(buffer[offset + 7]) << 56);
    offset += 8;
    double value;
    std::memcpy(&value, &bits, sizeof(double));
    return value;
}

glm::vec3 NativeFormat::readVec3(const std::vector<uint8_t>& buffer, size_t& offset)
{
    return glm::vec3(
        readFloat(buffer, offset),
        readFloat(buffer, offset),
        readFloat(buffer, offset)
    );
}

glm::dvec3 NativeFormat::readDVec3(const std::vector<uint8_t>& buffer, size_t& offset)
{
    return glm::dvec3(
        readDouble(buffer, offset),
        readDouble(buffer, offset),
        readDouble(buffer, offset)
    );
}

std::string NativeFormat::readString(const std::vector<uint8_t>& buffer, size_t& offset)
{
    uint32_t length = readUint32(buffer, offset);
    // CRITICAL FIX: Throw exception on buffer underflow instead of silently returning empty string
    if (offset + length > buffer.size()) {
        throw std::runtime_error("Buffer underflow in readString: offset " + 
            std::to_string(offset) + " + length " + std::to_string(length) + 
            " > size " + std::to_string(buffer.size()));
    }
    std::string str(buffer.begin() + offset, buffer.begin() + offset + length);
    offset += length;
    return str;
}

std::string NativeFormat::getCurrentTimestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    std::ostringstream ss;
    ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// SimpleArchive implementation
bool SimpleArchive::write(const std::string& filename, const std::vector<Entry>& entries)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Write magic number
    uint32_t magic = ARCHIVE_MAGIC;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    
    // Write entry count
    uint32_t count = entries.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write entries
    for (const auto& entry : entries) {
        // Name length and name
        uint32_t nameLen = entry.name.length();
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(entry.name.c_str(), nameLen);
        
        // Data length and data
        uint32_t dataLen = entry.data.size();
        file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));
        if (!entry.data.empty()) {
            file.write(reinterpret_cast<const char*>(entry.data.data()), dataLen);
        }
    }
    
    return true;
}

bool SimpleArchive::read(const std::string& filename, std::vector<Entry>& entries)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    // Read and verify magic number
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != ARCHIVE_MAGIC) return false;
    
    // Read entry count
    uint32_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    // Read entries
    for (uint32_t i = 0; i < count; i++) {
        Entry entry;
        
        // Name
        uint32_t nameLen;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        entry.name.resize(nameLen);
        file.read(&entry.name[0], nameLen);
        
        // Data
        uint32_t dataLen;
        file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));
        entry.data.resize(dataLen);
        if (dataLen > 0) {
            file.read(reinterpret_cast<char*>(entry.data.data()), dataLen);
        }
        
        entries.push_back(entry);
    }
    
    return true;
}

} // namespace dc
