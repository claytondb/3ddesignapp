/**
 * @file PLYImporter.h
 * @brief Stanford PLY file format importer
 * 
 * Supports ASCII and binary (little/big endian) PLY formats.
 * Parses the flexible PLY header to extract vertex and face data.
 */

#pragma once

#include "../geometry/MeshData.h"

#include <string>
#include <filesystem>
#include <istream>
#include <vector>
#include <variant>

namespace dc3d {
namespace io {

/**
 * @brief Options for PLY import
 */
struct PLYImportOptions {
    /// Compute vertex normals if not present in file
    bool computeNormalsIfMissing = true;
    
    /// Import vertex colors if present
    bool importColors = true;
    
    /// Report progress for files larger than this many elements
    size_t progressThreshold = 1000000;
};

/**
 * @brief PLY file importer
 * 
 * Supports:
 * - ASCII format
 * - Binary little-endian format
 * - Binary big-endian format
 * - Flexible property definitions
 * - Vertex positions (x, y, z)
 * - Vertex normals (nx, ny, nz)
 * - Face indices (vertex_indices or vertex_index)
 * - Vertex colors (red, green, blue)
 * 
 * Limitations:
 * - Only vertex and face elements are supported
 * - Edge elements are ignored
 * - List properties other than face indices are ignored
 */
class PLYImporter {
public:
    PLYImporter() = default;
    ~PLYImporter() = default;
    
    // Non-copyable
    PLYImporter(const PLYImporter&) = delete;
    PLYImporter& operator=(const PLYImporter&) = delete;
    
    /**
     * @brief Import PLY file from disk
     * @param path Path to PLY file
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> import(
        const std::filesystem::path& path,
        const PLYImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Import PLY from input stream
     * @param stream Input stream containing PLY data
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> importFromStream(
        std::istream& stream,
        const PLYImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);
    
    /**
     * @brief Import PLY from memory buffer
     * @param data Pointer to PLY data
     * @param size Size of data in bytes
     * @param options Import options
     * @param progress Optional progress callback
     * @return Result containing MeshData or error message
     */
    static geometry::Result<geometry::MeshData> importFromMemory(
        const void* data,
        size_t size,
        const PLYImportOptions& options = {},
        geometry::ProgressCallback progress = nullptr);

private:
    /// PLY data format types
    enum class Format {
        ASCII,
        BinaryLittleEndian,
        BinaryBigEndian
    };
    
    /// PLY property data types
    enum class DataType {
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Float32,
        Float64,
        Unknown
    };
    
    /// Property definition
    struct Property {
        std::string name;
        DataType type = DataType::Unknown;
        
        // For list properties
        bool isList = false;
        DataType listSizeType = DataType::Unknown;
        DataType listElementType = DataType::Unknown;
    };
    
    /// Element definition
    struct Element {
        std::string name;
        size_t count = 0;
        std::vector<Property> properties;
    };
    
    /// PLY header information
    struct Header {
        Format format = Format::ASCII;
        std::vector<Element> elements;
        
        const Element* findElement(const std::string& name) const {
            for (const auto& e : elements) {
                if (e.name == name) return &e;
            }
            return nullptr;
        }
    };
    
    /// Parse PLY header
    static geometry::Result<Header> parseHeader(std::istream& stream);
    
    /// Parse property type from string
    static DataType parseDataType(const std::string& typeStr);
    
    /// Get size of data type in bytes
    static size_t dataTypeSize(DataType type);
    
    /// Read ASCII data
    static geometry::Result<geometry::MeshData> readASCII(
        std::istream& stream,
        const Header& header,
        const PLYImportOptions& options,
        geometry::ProgressCallback progress);
    
    /// Read binary data
    static geometry::Result<geometry::MeshData> readBinary(
        std::istream& stream,
        const Header& header,
        const PLYImportOptions& options,
        geometry::ProgressCallback progress);
    
    /// Read a value of given type from binary stream
    template<typename T>
    static T readBinaryValue(std::istream& stream, bool swapBytes);
    
    /// Read a value of given DataType from binary stream as double
    static double readBinaryAsDouble(std::istream& stream, DataType type, bool swapBytes);
    
    /// Read a value of given DataType from binary stream as integer
    static int64_t readBinaryAsInt(std::istream& stream, DataType type, bool swapBytes);
    
    /// Swap bytes for endianness conversion
    static void swapBytes(void* data, size_t size);
};

} // namespace io
} // namespace dc3d
