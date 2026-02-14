/**
 * @file OBJImporter.cpp
 * @brief Implementation of Wavefront OBJ file format importer
 */

#include "OBJImporter.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

namespace dc3d {
namespace io {

namespace {

// Trim whitespace from string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// Hash function for vertex key (position + texcoord + normal indices)
struct VertexKey {
    int posIdx;
    int texIdx;
    int normIdx;
    
    bool operator==(const VertexKey& other) const {
        return posIdx == other.posIdx && 
               texIdx == other.texIdx && 
               normIdx == other.normIdx;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        // Combine hashes
        size_t h = std::hash<int>{}(k.posIdx);
        h ^= std::hash<int>{}(k.texIdx) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(k.normIdx) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

} // anonymous namespace

geometry::Result<geometry::MeshData> OBJImporter::import(
    const std::filesystem::path& path,
    const OBJImportOptions& options,
    geometry::ProgressCallback progress) {
    
    std::string fileName = path.filename().string();
    
    // Check file exists
    if (!std::filesystem::exists(path)) {
        return geometry::Result<geometry::MeshData>::failure(
            "File not found: \"" + fileName + "\"\n"
            "Path: " + path.string() + "\n"
            "Please check that the file exists and the path is correct.");
    }
    
    // Get file size
    std::error_code ec;
    auto fileSize = std::filesystem::file_size(path, ec);
    if (ec) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot read file: \"" + fileName + "\"\n"
            "Error: " + ec.message() + "\n"
            "Check that you have permission to read this file.");
    }
    
    if (fileSize == 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "File is empty: \"" + fileName + "\"\n"
            "The file contains no data. It may be corrupted or incomplete.");
    }
    
    // Open file
    std::ifstream file(path);
    if (!file) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot open file: \"" + fileName + "\"\n"
            "The file may be in use by another application or you may not have read permission.");
    }
    
    return importFromStream(file, options, progress);
}

geometry::Result<geometry::MeshData> OBJImporter::importFromStream(
    std::istream& stream,
    const OBJImportOptions& options,
    geometry::ProgressCallback progress) {
    
    // Temporary storage for OBJ data
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    
    // Face indices (each face is a vector of VertexKeys)
    std::vector<std::vector<VertexKey>> faces;
    
    // Estimate file size for progress
    stream.seekg(0, std::ios::end);
    auto fileSize = stream.tellg();
    stream.seekg(0);
    
    // Rough estimate: 30 bytes per line average
    size_t estimatedLines = static_cast<size_t>(fileSize) / 30;
    size_t currentLine = 0;
    
    bool reportProgress = progress && estimatedLines > options.progressThreshold;
    
    // Reserve approximate space
    positions.reserve(estimatedLines / 4);
    faces.reserve(estimatedLines / 4);
    
    std::string line;
    size_t lineNumber = 0;
    
    while (std::getline(stream, line)) {
        ++lineNumber;
        ++currentLine;
        
        // Progress reporting
        if (reportProgress && (currentLine % 100000 == 0)) {
            float prog = static_cast<float>(stream.tellg()) / fileSize;
            if (!progress(prog * 0.5f)) {  // First half is parsing
                return geometry::Result<geometry::MeshData>::failure(
                    "Import cancelled by user.");
            }
        }
        
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;  // Skip empty lines and comments
        }
        
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        if (keyword == "v") {
            // Vertex position
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            
            if (iss.fail()) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Invalid vertex coordinates. Expected: v x y z\n"
                    "Line content: " + line);
            }
            
            positions.push_back(pos);
        }
        else if (keyword == "vn") {
            // Vertex normal
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            
            if (iss.fail()) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Invalid vertex normal. Expected: vn nx ny nz\n"
                    "Line content: " + line);
            }
            
            normals.push_back(norm);
        }
        else if (keyword == "vt") {
            // Texture coordinate
            if (options.importUVs) {
                glm::vec2 uv;
                iss >> uv.x >> uv.y;
                
                if (iss.fail()) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Parse error at line " + std::to_string(lineNumber) + ":\n"
                        "Invalid texture coordinate. Expected: vt u v\n"
                        "Line content: " + line);
                }
                
                if (options.flipV) {
                    uv.y = 1.0f - uv.y;
                }
                
                texCoords.push_back(uv);
            }
        }
        else if (keyword == "f") {
            // Face
            std::vector<VertexKey> faceVertices;
            std::string vertexSpec;
            
            while (iss >> vertexSpec) {
                int vIdx = 0, vtIdx = 0, vnIdx = 0;
                
                if (!parseFaceVertex(vertexSpec, vIdx, vtIdx, vnIdx)) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Parse error at line " + std::to_string(lineNumber) + ":\n"
                        "Invalid face vertex format: '" + vertexSpec + "'\n"
                        "Expected format: v, v/vt, v/vt/vn, or v//vn");
                }
                
                // Handle negative (relative) indices
                if (vIdx < 0) {
                    vIdx = static_cast<int>(positions.size()) + vIdx + 1;
                }
                if (vtIdx < 0) {
                    vtIdx = static_cast<int>(texCoords.size()) + vtIdx + 1;
                }
                if (vnIdx < 0) {
                    vnIdx = static_cast<int>(normals.size()) + vnIdx + 1;
                }
                
                // HIGH FIX: Validate indices - must be positive after relative conversion
                if (vIdx <= 0 || vIdx > static_cast<int>(positions.size())) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Parse error at line " + std::to_string(lineNumber) + ":\n"
                        "Vertex index " + std::to_string(vIdx) + " is out of range.\n"
                        "Valid range: 1 to " + std::to_string(positions.size()) + "\n"
                        "The face references a vertex that hasn't been defined yet.");
                }
                // Also validate texture and normal indices if specified
                if (vtIdx != 0 && (vtIdx < 0 || vtIdx > static_cast<int>(texCoords.size()))) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Parse error at line " + std::to_string(lineNumber) + ":\n"
                        "Texture coordinate index " + std::to_string(vtIdx) + " is out of range.\n"
                        "Valid range: 1 to " + std::to_string(texCoords.size()));
                }
                if (vnIdx != 0 && (vnIdx < 0 || vnIdx > static_cast<int>(normals.size()))) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Parse error at line " + std::to_string(lineNumber) + ":\n"
                        "Normal index " + std::to_string(vnIdx) + " is out of range.\n"
                        "Valid range: 1 to " + std::to_string(normals.size()));
                }
                
                faceVertices.push_back({vIdx, vtIdx, vnIdx});
            }
            
            if (faceVertices.size() < 3) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Face has only " + std::to_string(faceVertices.size()) + " vertices.\n"
                    "A face must have at least 3 vertices to form a polygon.");
            }
            
            faces.push_back(std::move(faceVertices));
        }
        // Skip other keywords (o, g, s, usemtl, mtllib, etc.)
    }
    
    if (positions.empty()) {
        return geometry::Result<geometry::MeshData>::failure(
            "No vertices found in OBJ file.\n"
            "The file contains no 'v' (vertex position) entries.\n"
            "Check that this is a valid Wavefront OBJ file.");
    }
    
    if (faces.empty()) {
        return geometry::Result<geometry::MeshData>::failure(
            "No faces found in OBJ file.\n"
            "Found " + std::to_string(positions.size()) + " vertices but no 'f' (face) entries.\n"
            "The file may be a point cloud rather than a mesh.");
    }
    
    // Build mesh - deduplicate vertices with same pos/tex/norm combination
    geometry::MeshData mesh;
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;
    
    // Estimate vertex count (upper bound is 3 * face count for all triangles)
    size_t estimatedVertices = 0;
    for (const auto& face : faces) {
        estimatedVertices += face.size();
    }
    mesh.reserveVertices(estimatedVertices);
    
    // Estimate triangle count
    size_t estimatedTriangles = 0;
    for (const auto& face : faces) {
        if (options.triangulate && face.size() > 3) {
            estimatedTriangles += face.size() - 2;  // Fan triangulation
        } else {
            estimatedTriangles += 1;
        }
    }
    mesh.reserveFaces(estimatedTriangles);
    
    bool hasNormals = !normals.empty();
    bool hasUVs = !texCoords.empty() && options.importUVs;
    
    // Process faces
    size_t faceIndex = 0;
    for (const auto& face : faces) {
        ++faceIndex;
        
        // Progress reporting (second half)
        if (reportProgress && (faceIndex % 100000 == 0)) {
            float prog = 0.5f + 0.5f * static_cast<float>(faceIndex) / faces.size();
            if (!progress(prog)) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Import cancelled by user.");
            }
        }
        
        // Convert face vertices to mesh vertex indices
        std::vector<uint32_t> meshIndices;
        meshIndices.reserve(face.size());
        
        for (const auto& vk : face) {
            // Check if we already have this vertex combination
            auto it = vertexMap.find(vk);
            if (it != vertexMap.end()) {
                meshIndices.push_back(it->second);
            } else {
                // Create new vertex
                glm::vec3 pos = positions[vk.posIdx - 1];  // OBJ is 1-indexed
                uint32_t newIdx;
                
                if (hasNormals && vk.normIdx > 0 && 
                    vk.normIdx <= static_cast<int>(normals.size())) {
                    glm::vec3 norm = normals[vk.normIdx - 1];
                    newIdx = mesh.addVertex(pos, norm);
                } else {
                    newIdx = mesh.addVertex(pos);
                }
                
                // Add UV if available
                if (hasUVs && vk.texIdx > 0 && 
                    vk.texIdx <= static_cast<int>(texCoords.size())) {
                    // LOW FIX: Use resize() instead of push_back in loop (O(1) vs O(n²))
                    if (newIdx >= mesh.uvs().size()) {
                        mesh.uvs().resize(newIdx + 1, glm::vec2(0.0f));
                    }
                    mesh.uvs()[newIdx] = texCoords[vk.texIdx - 1];
                }
                
                vertexMap[vk] = newIdx;
                meshIndices.push_back(newIdx);
            }
        }
        
        // Triangulate face
        if (meshIndices.size() == 3) {
            // Triangle - add directly
            mesh.addFace(meshIndices[0], meshIndices[1], meshIndices[2]);
        }
        else if (meshIndices.size() == 4 && options.triangulate) {
            // Quad - split into 2 triangles (fan from first vertex)
            mesh.addFace(meshIndices[0], meshIndices[1], meshIndices[2]);
            mesh.addFace(meshIndices[0], meshIndices[2], meshIndices[3]);
        }
        else if (meshIndices.size() > 4 && options.triangulate) {
            // Polygon - fan triangulation from first vertex
            for (size_t i = 1; i < meshIndices.size() - 1; ++i) {
                mesh.addFace(meshIndices[0], meshIndices[i], meshIndices[i + 1]);
            }
        }
        else if (meshIndices.size() > 3) {
            // Skip non-triangulated polygons
            // Could add as first triangle only, but better to fail explicitly
        }
    }
    
    // Compute normals if missing
    if (options.computeNormalsIfMissing && !mesh.hasNormals()) {
        mesh.computeNormals();
    }
    
    mesh.shrinkToFit();
    
    if (progress) {
        progress(1.0f);
    }
    
    return geometry::Result<geometry::MeshData>::success(std::move(mesh));
}

geometry::Result<geometry::MeshData> OBJImporter::importFromMemory(
    const void* data,
    size_t size,
    const OBJImportOptions& options,
    geometry::ProgressCallback progress) {
    
    if (!data || size == 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot import from memory: data buffer is empty or null.");
    }
    
    std::string dataStr(static_cast<const char*>(data), size);
    std::istringstream stream(dataStr);
    
    return importFromStream(stream, options, progress);
}

bool OBJImporter::parseFaceVertex(const std::string& spec,
                                   int& vertexIdx,
                                   int& texCoordIdx,
                                   int& normalIdx) {
    vertexIdx = 0;
    texCoordIdx = 0;
    normalIdx = 0;
    
    if (spec.empty()) {
        return false;
    }
    
    // Find slashes
    size_t slash1 = spec.find('/');
    
    if (slash1 == std::string::npos) {
        // Just vertex index: v
        try {
            vertexIdx = std::stoi(spec);
        } catch (...) {
            return false;
        }
        return true;
    }
    
    // Parse vertex index
    try {
        vertexIdx = std::stoi(spec.substr(0, slash1));
    } catch (...) {
        return false;
    }
    
    size_t slash2 = spec.find('/', slash1 + 1);
    
    if (slash2 == std::string::npos) {
        // Vertex and texture: v/vt
        std::string texStr = spec.substr(slash1 + 1);
        if (!texStr.empty()) {
            try {
                texCoordIdx = std::stoi(texStr);
            } catch (...) {
                return false;
            }
        }
        return true;
    }
    
    // Three parts: v/vt/vn or v//vn
    std::string texStr = spec.substr(slash1 + 1, slash2 - slash1 - 1);
    std::string normStr = spec.substr(slash2 + 1);
    
    if (!texStr.empty()) {
        try {
            texCoordIdx = std::stoi(texStr);
        } catch (...) {
            return false;
        }
    }
    
    if (!normStr.empty()) {
        try {
            normalIdx = std::stoi(normStr);
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

} // namespace io
} // namespace dc3d
