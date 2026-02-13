/**
 * @file MeshImporter.h
 * @brief Mesh file import functionality
 * 
 * Provides importing of common mesh formats (STL, OBJ, PLY).
 */

#ifndef DC3D_IO_MESHIMPORTER_H
#define DC3D_IO_MESHIMPORTER_H

#include <string>
#include <memory>
#include <optional>
#include <vector>

namespace dc3d {
namespace geometry {
class MeshData;
}

namespace io {

/**
 * @struct ImportOptions
 * @brief Options for mesh import operations
 */
struct ImportOptions
{
    bool computeNormals = true;     ///< Recompute normals after import
    bool mergeVertices = true;      ///< Merge duplicate vertices
    double mergeTolerance = 1e-6;   ///< Tolerance for vertex merging
};

/**
 * @struct ImportResult
 * @brief Result of a mesh import operation
 */
struct ImportResult
{
    std::unique_ptr<geometry::MeshData> mesh;
    std::string error;
    size_t vertexCount = 0;
    size_t faceCount = 0;
    double loadTimeMs = 0.0;
    
    bool ok() const { return mesh != nullptr && error.empty(); }
    explicit operator bool() const { return ok(); }
};

/**
 * @class MeshImporter
 * @brief Static class for importing mesh files
 * 
 * Supported formats:
 * - STL (ASCII and Binary)
 * - OBJ (Wavefront, with MTL support)
 * - PLY (ASCII and Binary)
 */
class MeshImporter
{
public:
    /**
     * @brief Import a mesh from file
     * @param filePath Path to the mesh file
     * @param options Import options
     * @return ImportResult containing the mesh or error
     */
    static ImportResult import(const std::string& filePath, 
                               const ImportOptions& options = ImportOptions());
    
    /**
     * @brief Check if a file format is supported
     * @param extension File extension (e.g., ".stl", ".obj")
     * @return true if the format is supported
     */
    static bool isSupported(const std::string& extension);
    
    /**
     * @brief Get list of supported file extensions
     */
    static std::vector<std::string> supportedExtensions();

private:
    static ImportResult importSTL(const std::string& filePath, const ImportOptions& options);
    static ImportResult importOBJ(const std::string& filePath, const ImportOptions& options);
    static ImportResult importPLY(const std::string& filePath, const ImportOptions& options);
};

} // namespace io
} // namespace dc3d

#endif // DC3D_IO_MESHIMPORTER_H
