/**
 * @file STLImporter.cpp
 * @brief Implementation of STL file format importer
 */

#include "STLImporter.h"

#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <cstdint>

namespace dc3d {
namespace io {

namespace {

// Binary STL header size
constexpr size_t STL_HEADER_SIZE = 80;

// Size of a single triangle in binary STL (normal + 3 vertices + attribute)
constexpr size_t STL_TRIANGLE_SIZE = 50;  // 12 + 12*3 + 2 = 50 bytes

// Read a little-endian float from stream
float readFloat(std::istream& stream) {
    float value;
    stream.read(reinterpret_cast<char*>(&value), sizeof(float));
    return value;
}

// Read a little-endian uint32 from stream
uint32_t readUint32(std::istream& stream) {
    uint32_t value;
    stream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    return value;
}

// Read a little-endian uint16 from stream
uint16_t readUint16(std::istream& stream) {
    uint16_t value;
    stream.read(reinterpret_cast<char*>(&value), sizeof(uint16_t));
    return value;
}

// Read vec3 from stream (3 floats)
glm::vec3 readVec3(std::istream& stream) {
    glm::vec3 v;
    v.x = readFloat(stream);
    v.y = readFloat(stream);
    v.z = readFloat(stream);
    return v;
}

// Trim whitespace from string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// Convert string to lowercase
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // anonymous namespace

geometry::Result<geometry::MeshData> STLImporter::import(
    const std::filesystem::path& path,
    const STLImportOptions& options,
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
    
    // Check minimum size for valid STL
    if (fileSize < STL_HEADER_SIZE + 4) {
        return geometry::Result<geometry::MeshData>::failure(
            "File too small: \"" + fileName + "\" (" + std::to_string(fileSize) + " bytes)\n"
            "A valid STL file must be at least " + std::to_string(STL_HEADER_SIZE + 4) + " bytes.\n"
            "The file may be truncated or corrupted.");
    }
    
    // Detect format
    auto isBinaryOpt = detectBinaryFormat(path);
    if (!isBinaryOpt) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot determine STL format: \"" + fileName + "\"\n"
            "The file header is unreadable. It may be corrupted or not a valid STL file.");
    }
    
    bool isBinary = *isBinaryOpt;
    
    // Open file
    std::ios_base::openmode mode = std::ios::in;
    if (isBinary) {
        mode |= std::ios::binary;
    }
    
    std::ifstream file(path, mode);
    if (!file) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot open file: \"" + fileName + "\"\n"
            "The file may be in use by another application or you may not have read permission.");
    }
    
    return importFromStream(file, isBinary, options, progress);
}

geometry::Result<geometry::MeshData> STLImporter::importFromStream(
    std::istream& stream,
    bool isBinary,
    const STLImportOptions& options,
    geometry::ProgressCallback progress) {
    
    if (isBinary) {
        return importBinary(stream, options, progress);
    } else {
        return importASCII(stream, options, progress);
    }
}

geometry::Result<geometry::MeshData> STLImporter::importFromMemory(
    const void* data,
    size_t size,
    const STLImportOptions& options,
    geometry::ProgressCallback progress) {
    
    if (!data || size == 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot import from memory: data buffer is empty or null.");
    }
    
    if (size < STL_HEADER_SIZE + 4) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot import from memory: data too small (" + std::to_string(size) + " bytes).\n"
            "A valid STL requires at least " + std::to_string(STL_HEADER_SIZE + 4) + " bytes.");
    }
    
    // Create memory stream
    std::string dataStr(static_cast<const char*>(data), size);
    std::istringstream stream(dataStr, std::ios::binary);
    
    // Detect format
    bool isBinary = detectBinaryFormat(stream);
    stream.seekg(0);  // Reset to beginning
    
    return importFromStream(stream, isBinary, options, progress);
}

std::optional<bool> STLImporter::detectBinaryFormat(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    
    return detectBinaryFormat(file);
}

bool STLImporter::detectBinaryFormat(std::istream& stream) {
    // Save current position
    auto startPos = stream.tellg();
    
    // Read first 80 characters (header for binary STL)
    char header[STL_HEADER_SIZE + 1];
    stream.read(header, STL_HEADER_SIZE);
    header[STL_HEADER_SIZE] = '\0';
    
    if (!stream) {
        stream.clear();
        stream.seekg(startPos);
        return false;  // File too small, assume ASCII
    }
    
    // Check if header starts with "solid" (ASCII STL marker)
    std::string headerStr(header);
    std::string headerLower = toLower(trim(headerStr));
    
    // Read triangle count (for binary)
    uint32_t triangleCount = readUint32(stream);
    
    // Get file size
    stream.seekg(0, std::ios::end);
    auto fileSize = stream.tellg();
    
    // Reset position
    stream.seekg(startPos);
    
    // Calculate expected binary file size
    size_t expectedBinarySize = STL_HEADER_SIZE + 4 + (triangleCount * STL_TRIANGLE_SIZE);
    
    // Heuristics:
    // 1. If file size matches binary size calculation, it's likely binary
    // 2. If header starts with "solid" and file size doesn't match, it's likely ASCII
    // 3. Check for non-printable characters in header
    
    // Check for non-printable characters (indicates binary)
    bool hasNonPrintable = false;
    for (int i = 0; i < 80; ++i) {
        unsigned char c = static_cast<unsigned char>(header[i]);
        if (c != 0 && (c < 32 || c > 126)) {
            hasNonPrintable = true;
            break;
        }
    }
    
    // Decision logic
    if (static_cast<size_t>(fileSize) == expectedBinarySize && triangleCount > 0) {
        return true;  // Likely binary
    }
    
    if (headerLower.substr(0, 5) == "solid" && !hasNonPrintable) {
        // Additional check: look for "facet" keyword in ASCII STL
        stream.seekg(startPos);
        char buffer[256];
        stream.read(buffer, 255);
        buffer[255] = '\0';
        
        std::string content(buffer);
        stream.seekg(startPos);
        
        // ASCII STL should have "facet" keyword early
        if (content.find("facet") != std::string::npos ||
            content.find("endsolid") != std::string::npos) {
            return false;  // ASCII
        }
    }
    
    // Default to binary if file size matches or has non-printable chars
    return hasNonPrintable || 
           (static_cast<size_t>(fileSize) >= STL_HEADER_SIZE + 4 + STL_TRIANGLE_SIZE);
}

size_t STLImporter::getTriangleCount(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return 0;
    }
    
    // Check if binary
    if (!detectBinaryFormat(file)) {
        return 0;  // Can't get count from ASCII quickly
    }
    
    // Skip header
    file.seekg(STL_HEADER_SIZE);
    
    // Read triangle count
    return readUint32(file);
}

geometry::Result<geometry::MeshData> STLImporter::importASCII(
    std::istream& stream,
    const STLImportOptions& options,
    geometry::ProgressCallback progress) {
    
    geometry::MeshData mesh;
    
    std::string line;
    std::string token;
    
    glm::vec3 faceNormal(0.0f);
    glm::vec3 vertices[3];
    int vertexIndex = 0;
    bool inFacet = false;
    size_t faceCount = 0;
    size_t lineNumber = 0;
    
    // Estimate total lines for progress (rough guess)
    stream.seekg(0, std::ios::end);
    auto fileSize = stream.tellg();
    stream.seekg(0);
    
    // Rough estimate: ~50 bytes per triangle in ASCII
    size_t estimatedTriangles = static_cast<size_t>(fileSize) / 200;
    bool reportProgress = progress && estimatedTriangles > options.progressThreshold;
    
    // Reserve approximate space
    if (estimatedTriangles > 0) {
        mesh.reserveVertices(estimatedTriangles * 3);
        mesh.reserveFaces(estimatedTriangles);
    }
    
    while (std::getline(stream, line)) {
        ++lineNumber;
        
        line = trim(line);
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        iss >> token;
        token = toLower(token);
        
        if (token == "solid") {
            // Start of solid - ignore name
            continue;
        }
        else if (token == "endsolid") {
            // End of solid
            break;
        }
        else if (token == "facet") {
            // facet normal ni nj nk
            inFacet = true;
            vertexIndex = 0;
            
            iss >> token;  // "normal"
            iss >> faceNormal.x >> faceNormal.y >> faceNormal.z;
        }
        else if (token == "outer") {
            // outer loop - just skip
            continue;
        }
        else if (token == "vertex") {
            if (!inFacet) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Found 'vertex' outside of a facet block.\n"
                    "Expected 'facet normal' before vertex definitions.");
            }
            if (vertexIndex >= 3) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Too many vertices in facet (found more than 3).\n"
                    "STL format only supports triangular faces.");
            }
            
            iss >> vertices[vertexIndex].x 
                >> vertices[vertexIndex].y 
                >> vertices[vertexIndex].z;
            
            if (iss.fail()) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Invalid vertex coordinates. Expected 3 numeric values.\n"
                    "Line content: " + line);
            }
            ++vertexIndex;
        }
        else if (token == "endloop") {
            // End of vertex loop
            continue;
        }
        else if (token == "endfacet") {
            if (!inFacet) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Found 'endfacet' without matching 'facet' keyword.");
            }
            if (vertexIndex != 3) {
                return geometry::Result<geometry::MeshData>::failure(
                    "Parse error at line " + std::to_string(lineNumber) + ":\n"
                    "Incomplete facet - found " + std::to_string(vertexIndex) + " vertices, expected 3.\n"
                    "Each triangle in STL must have exactly 3 vertices.");
            }
            
            // Add the triangle
            uint32_t i0 = mesh.addVertex(vertices[0]);
            uint32_t i1 = mesh.addVertex(vertices[1]);
            uint32_t i2 = mesh.addVertex(vertices[2]);
            mesh.addFace(i0, i1, i2);
            
            inFacet = false;
            ++faceCount;
            
            // Progress reporting
            if (reportProgress && (faceCount % 100000 == 0)) {
                float prog = static_cast<float>(faceCount) / estimatedTriangles;
                if (!progress(std::min(prog, 0.95f))) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Import cancelled by user.");
                }
            }
        }
    }
    
    if (mesh.isEmpty()) {
        return geometry::Result<geometry::MeshData>::failure(
            "No valid triangles found in ASCII STL file.\n"
            "The file may be empty, or it may not be a valid STL file.\n"
            "Check that the file contains 'facet' and 'vertex' definitions.");
    }
    
    // Post-processing
    if (options.mergeVertexTolerance > 0) {
        mesh.mergeDuplicateVertices(options.mergeVertexTolerance, 
            reportProgress ? progress : nullptr);
    }
    
    if (options.computeNormals) {
        mesh.computeNormals();
    }
    
    mesh.shrinkToFit();
    
    // CRITICAL FIX: Validate ASCII mesh before returning to catch any corruption
    if (!mesh.isValid()) {
        return geometry::Result<geometry::MeshData>::failure(
            "Imported mesh validation failed.\n"
            "The mesh data appears to be corrupted or contains invalid geometry.\n"
            "Try re-exporting the STL file from the original application.");
    }
    
    if (progress) {
        progress(1.0f);
    }
    
    return geometry::Result<geometry::MeshData>::success(std::move(mesh));
}

geometry::Result<geometry::MeshData> STLImporter::importBinary(
    std::istream& stream,
    const STLImportOptions& options,
    geometry::ProgressCallback progress) {
    
    geometry::MeshData mesh;
    
    // Read header (80 bytes)
    char header[STL_HEADER_SIZE];
    stream.read(header, STL_HEADER_SIZE);
    if (!stream) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot read STL header (first 80 bytes).\n"
            "The file may be corrupted or truncated.");
    }
    
    // Read triangle count
    uint32_t triangleCount = readUint32(stream);
    if (!stream) {
        return geometry::Result<geometry::MeshData>::failure(
            "Cannot read triangle count from STL header.\n"
            "The file may be corrupted or truncated.");
    }
    
    if (triangleCount == 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "STL file contains no triangles.\n"
            "The file declares 0 triangles - it may be empty or corrupted.");
    }
    
    // Sanity check for triangle count (prevent bad allocations)
    constexpr size_t MAX_TRIANGLES = 100000000;  // 100M triangles max
    // Check for overflow before multiplication (CRITICAL FIX: Integer overflow prevention)
    if (triangleCount > MAX_TRIANGLES || triangleCount > SIZE_MAX / 3) {
        return geometry::Result<geometry::MeshData>::failure(
            "Triangle count too large: " + std::to_string(triangleCount) + " triangles.\n"
            "Maximum supported: " + std::to_string(MAX_TRIANGLES) + " triangles.\n"
            "Try decimating the mesh in the original application before importing.");
    }
    
    // Validate file has enough data for declared triangle count (CRITICAL FIX: File size validation)
    size_t expectedSize = STL_HEADER_SIZE + 4 + (static_cast<size_t>(triangleCount) * STL_TRIANGLE_SIZE);
    stream.seekg(0, std::ios::end);
    auto actualFileSize = stream.tellg();
    if (actualFileSize < 0 || static_cast<size_t>(actualFileSize) < expectedSize) {
        size_t actualSize = actualFileSize > 0 ? static_cast<size_t>(actualFileSize) : 0;
        size_t missingBytes = expectedSize - actualSize;
        return geometry::Result<geometry::MeshData>::failure(
            "STL file is truncated.\n"
            "File declares " + std::to_string(triangleCount) + " triangles, "
            "but file is missing " + std::to_string(missingBytes) + " bytes of data.\n"
            "The file may have been incompletely downloaded or copied.");
    }
    stream.seekg(STL_HEADER_SIZE + 4);  // Reset to start of triangle data
    
    bool reportProgress = progress && triangleCount > options.progressThreshold;
    
    // Pre-allocate (STL files have 3 vertices per triangle, with duplicates)
    mesh.reserveVertices(triangleCount * 3);
    mesh.reserveFaces(triangleCount);
    
    // Read triangles
    for (uint32_t t = 0; t < triangleCount; ++t) {
        // LOW FIX: Skip face normal directly instead of reading into unused variable
        // Skip 12 bytes (normal vector) - we recompute normals later
        stream.seekg(12, std::ios::cur);
        
        // Read 3 vertices (36 bytes)
        glm::vec3 v0 = readVec3(stream);
        glm::vec3 v1 = readVec3(stream);
        glm::vec3 v2 = readVec3(stream);
        
        // LOW FIX: Skip attribute byte count directly (2 bytes) - usually 0
        stream.seekg(2, std::ios::cur);
        
        if (!stream) {
            return geometry::Result<geometry::MeshData>::failure(
                "Failed to read triangle " + std::to_string(t + 1) + " of " + std::to_string(triangleCount) + ".\n"
                "The file may be truncated or corrupted.");
        }
        
        // Add vertices and face
        uint32_t i0 = mesh.addVertex(v0);
        uint32_t i1 = mesh.addVertex(v1);
        uint32_t i2 = mesh.addVertex(v2);
        mesh.addFace(i0, i1, i2);
        
        // Progress reporting
        if (reportProgress && (t % 100000 == 0)) {
            float prog = static_cast<float>(t) / triangleCount;
            if (!progress(prog * 0.8f)) {  // Reserve 20% for post-processing
                return geometry::Result<geometry::MeshData>::failure(
                    "Import cancelled");
            }
        }
    }
    
    // Post-processing
    if (options.mergeVertexTolerance > 0) {
        auto progressWrapper = reportProgress ? 
            [&progress](float p) { return progress(0.8f + p * 0.15f); } : 
            geometry::ProgressCallback{};
        
        mesh.mergeDuplicateVertices(options.mergeVertexTolerance, progressWrapper);
    }
    
    if (options.computeNormals) {
        mesh.computeNormals();
    }
    
    mesh.shrinkToFit();
    
    // CRITICAL FIX: Validate binary mesh before returning to catch any corruption
    if (!mesh.isValid()) {
        return geometry::Result<geometry::MeshData>::failure(
            "Imported mesh validation failed.\n"
            "The mesh contains " + std::to_string(mesh.faceCount()) + " triangles but the data appears invalid.\n"
            "This may indicate file corruption. Try re-exporting from the original application.");
    }
    
    if (progress) {
        progress(1.0f);
    }
    
    return geometry::Result<geometry::MeshData>::success(std::move(mesh));
}

} // namespace io
} // namespace dc3d
