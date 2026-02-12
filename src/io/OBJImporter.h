/**
 * @file OBJImporter.h
 * @brief Wavefront OBJ file format importer
 * 
 * Supports importing vertex positions, normals, texture coordinates,
 * and faces (triangles and quads with automatic triangulation).
 */

#pragma once

#include "../geometry/MeshData.h"

#include <string>
#include <filesystem>
#include <istream>

namespace dc3d {
namespace io {

/**
 * @brief Options for OBJ import
 */
struct OBJImportOptions {
    /// Compute vertex normals if not present in file
    bool computeNormalsIfMissing = true;
    
    /// Import texture coordinates (UVs)
    bool importUVs = true;
    
    /// Triangulate quads and larger polygons
    bool triangulate = true;
    
    /// Report progress for files larger than this many faces
    size_t progressThreshold = 1000000;
    
    /// Flip V texture coordinate (some exporters use different convention)
    bool flipV = false;
    
    /// Ignore materials (MTL files)
    bool ignoreMaterials = true;
};

/**
 * @brief OBJ file importer
 * 
 * Supports:
 * - Vertex positions (v x y z)
 * - Vertex normals (vn nx ny nz)
 * - Texture coordinates (vt u v)
 * - Faces (f v1 v2 v3... or f v1/vt1/vn1...)
 * - Triangles and quads (quads are triangulated)
 * - Negative indices (relative to end)
 * - Comments (#)
 * - Objects (o) and groups (g) - currently ignored
 * 
 * Does NOT support:
 * - Materials (usemtl, mtllib)
 * - Lines (l)
 * - Curves and surfaces
 * - Freeform geometry
 */
class OBJImporter {
public:
    OBJImporter() = default;
    ~OBJImporter() = default;
    
    // Non-copyable
    OBJImporter(const OBJImporter&) = delete;
    OBJImporter& operator=(const OBJImporter&) = delete;
    
    /**
     * @brief Import OBJ file from disk
     * @param path Path to OBJ file
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> import(
        const std::filesystem::path& path,
        const OBJImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Import OBJ from input stream
     * @param stream Input stream containing OBJ data
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> importFromStream(
        std::istream& stream,
        const OBJImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Import OBJ from memory buffer
     * @param data Pointer to OBJ data
     * @param size Size of data in bytes
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> importFromMemory(
        const void* data,
        size_t size,
        const OBJImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);

private:
    /**
     * @brief Parse face vertex specification (v, v/vt, v/vt/vn, or v//vn)
     * @param spec Face vertex specification string
     * @param vertexIdx Output vertex index (1-based, or negative)
     * @param texCoordIdx Output texture coordinate index (0 if not present)
     * @param normalIdx Output normal index (0 if not present)
     * @return true if parsed successfully
     */
    static bool parseFaceVertex(const std::string& spec,
                                int& vertexIdx,
                                int& texCoordIdx,
                                int& normalIdx);
};

} // namespace io
} // namespace dc3d
