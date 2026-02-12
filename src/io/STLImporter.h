/**
 * @file STLImporter.h
 * @brief STL file format importer (ASCII and binary)
 * 
 * Supports both ASCII and binary STL formats with automatic detection.
 * Handles large files efficiently with progress reporting.
 */

#pragma once

#include "../geometry/MeshData.h"

#include <string>
#include <filesystem>
#include <istream>

namespace dc3d {
namespace io {

/**
 * @brief Options for STL import
 */
struct STLImportOptions {
    /// Merge duplicate vertices within this tolerance (0 to disable)
    float mergeVertexTolerance = 1e-6f;
    
    /// Compute vertex normals after import
    bool computeNormals = true;
    
    /// Report progress for files larger than this many triangles
    size_t progressThreshold = 1000000;
};

/**
 * @brief STL file importer
 * 
 * Supports:
 * - ASCII STL format
 * - Binary STL format
 * - Auto-detection of format
 * - Progress reporting for large files
 * - Error handling with descriptive messages
 */
class STLImporter {
public:
    STLImporter() = default;
    ~STLImporter() = default;
    
    // Non-copyable
    STLImporter(const STLImporter&) = delete;
    STLImporter& operator=(const STLImporter&) = delete;
    
    /**
     * @brief Import STL file from disk
     * @param path Path to STL file
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> import(
        const std::filesystem::path& path,
        const STLImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Import STL from input stream
     * @param stream Input stream containing STL data
     * @param isBinary true for binary format, false for ASCII
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> importFromStream(
        std::istream& stream,
        bool isBinary,
        const STLImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Import STL from memory buffer
     * @param data Pointer to STL data
     * @param size Size of data in bytes
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> importFromMemory(
        const void* data,
        size_t size,
        const STLImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Detect if file is binary STL format
     * @param path Path to STL file
     * @return true if binary, false if ASCII, nullopt if error
     */
    static std::optional<bool> detectBinaryFormat(const std::filesystem::path& path);
    
    /**
     * @brief Detect if stream contains binary STL format
     * @param stream Input stream (will read and restore position)
     * @return true if binary, false if ASCII
     */
    static bool detectBinaryFormat(std::istream& stream);
    
    /**
     * @brief Get expected triangle count from binary STL
     * @param path Path to STL file
     * @return Triangle count or 0 if not binary/error
     */
    static size_t getTriangleCount(const std::filesystem::path& path);

private:
    static geometry::Result<geometry::MeshData> importASCII(
        std::istream& stream,
        const STLImportOptions& options,
        geometry::ProgressCallback progress);
    
    static geometry::Result<geometry::MeshData> importBinary(
        std::istream& stream,
        const STLImportOptions& options,
        geometry::ProgressCallback progress);
};

} // namespace io
} // namespace dc3d
