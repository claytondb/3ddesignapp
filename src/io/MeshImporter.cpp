/**
 * @file MeshImporter.cpp
 * @brief Implementation of MeshImporter class
 */

#include "MeshImporter.h"
#include "geometry/MeshData.h"

#include <algorithm>
#include <cctype>
#include <chrono>

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

ImportResult MeshImporter::importSTL(const std::string& filePath, const ImportOptions& /*options*/)
{
    ImportResult result;
    result.mesh = std::make_unique<geometry::MeshData>();
    
    // TODO: Implement STL parsing
    // For now, return empty mesh
    result.error = "STL import not yet implemented";
    result.mesh.reset();
    
    return result;
}

ImportResult MeshImporter::importOBJ(const std::string& filePath, const ImportOptions& /*options*/)
{
    ImportResult result;
    result.mesh = std::make_unique<geometry::MeshData>();
    
    // TODO: Implement OBJ parsing
    result.error = "OBJ import not yet implemented";
    result.mesh.reset();
    
    return result;
}

ImportResult MeshImporter::importPLY(const std::string& filePath, const ImportOptions& /*options*/)
{
    ImportResult result;
    result.mesh = std::make_unique<geometry::MeshData>();
    
    // TODO: Implement PLY parsing
    result.error = "PLY import not yet implemented";
    result.mesh.reset();
    
    return result;
}

} // namespace io
} // namespace dc3d
