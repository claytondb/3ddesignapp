/**
 * @file MeshImporter.cpp
 * @brief Implementation of MeshImporter class
 */

#include "MeshImporter.h"
#include "STLImporter.h"
#include "OBJImporter.h"
#include "PLYImporter.h"
#include "geometry/MeshData.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>

namespace dc3d {
namespace io {

namespace {

std::string toLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string getExtension(const std::string& filePath)
{
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos) {
        return "";
    }
    return toLower(filePath.substr(dotPos));
}

} // anonymous namespace

ImportResult MeshImporter::import(const std::string& filePath, const ImportOptions& options)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::string ext = getExtension(filePath);
    ImportResult result;
    
    if (ext == ".stl") {
        result = importSTL(filePath, options);
    } else if (ext == ".obj") {
        result = importOBJ(filePath, options);
    } else if (ext == ".ply") {
        result = importPLY(filePath, options);
    } else {
        result.error = "Unsupported file format: " + ext;
        return result;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.loadTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (result.mesh) {
        result.vertexCount = result.mesh->vertexCount();
        result.faceCount = result.mesh->faceCount();
        
        if (options.computeNormals) {
            result.mesh->computeNormals();
        }
    }
    
    return result;
}

bool MeshImporter::isSupported(const std::string& extension)
{
    std::string ext = toLower(extension);
    return ext == ".stl" || ext == ".obj" || ext == ".ply";
}

std::vector<std::string> MeshImporter::supportedExtensions()
{
    return {".stl", ".obj", ".ply"};
}

ImportResult MeshImporter::importSTL(const std::string& filePath, const ImportOptions& options)
{
    ImportResult result;
    
    // MEDIUM FIX: Forward to actual STLImporter instead of stub
    STLImportOptions stlOptions;
    stlOptions.computeNormals = options.computeNormals;
    stlOptions.mergeVertexTolerance = options.mergeVertices ? 1e-6f : 0.0f;
    
    auto stlResult = STLImporter::import(std::filesystem::path(filePath), stlOptions, nullptr);
    
    if (stlResult) {
        result.mesh = std::make_unique<geometry::MeshData>(std::move(*stlResult.value));
    } else {
        result.error = stlResult.error;
    }
    
    return result;
}

ImportResult MeshImporter::importOBJ(const std::string& filePath, const ImportOptions& options)
{
    ImportResult result;
    
    // MEDIUM FIX: Forward to actual OBJImporter instead of stub
    OBJImportOptions objOptions;
    objOptions.computeNormalsIfMissing = options.computeNormals;
    objOptions.triangulate = true;
    objOptions.importUVs = true;
    
    auto objResult = OBJImporter::import(std::filesystem::path(filePath), objOptions, nullptr);
    
    if (objResult) {
        result.mesh = std::make_unique<geometry::MeshData>(std::move(*objResult.value));
    } else {
        result.error = objResult.error;
    }
    
    return result;
}

ImportResult MeshImporter::importPLY(const std::string& filePath, const ImportOptions& options)
{
    ImportResult result;
    
    // MEDIUM FIX: Forward to actual PLYImporter instead of stub
    PLYImportOptions plyOptions;
    plyOptions.computeNormalsIfMissing = options.computeNormals;
    
    auto plyResult = PLYImporter::import(std::filesystem::path(filePath), plyOptions, nullptr);
    
    if (plyResult) {
        result.mesh = std::make_unique<geometry::MeshData>(std::move(*plyResult.value));
    } else {
        result.error = plyResult.error;
    }
    
    return result;
}

} // namespace io
} // namespace dc3d
