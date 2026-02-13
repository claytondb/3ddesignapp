#pragma once

#include "ExportOptions.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <glm/glm.hpp>

namespace dc {

// Forward declarations
class Model;
class Body;
class Face;
class Sketch;
class NURBSSurface;
class NURBSCurve;
class Project;

/**
 * File format version
 */
constexpr uint32_t DCA_FORMAT_VERSION = 1;
constexpr uint32_t DCA_MAGIC_NUMBER = 0x44434133;  // "DCA3"

/**
 * Chunk types in binary data
 */
enum class DCAChunkType : uint32_t {
    HEADER = 0x48445200,      // "HDR\0"
    MESH_DATA = 0x4D455348,   // "MESH"
    SURFACE_DATA = 0x53555246, // "SURF"
    CURVE_DATA = 0x43555256,  // "CURV"
    SKETCH_DATA = 0x534B4348,  // "SKCH"
    MATERIAL_DATA = 0x4D415452, // "MATR"
    END_OF_FILE = 0x454E4400   // "END\0"
};

/**
 * Project settings stored in manifest
 */
struct ProjectSettings {
    std::string name = "Untitled";
    std::string author;
    std::string description;
    std::string units = "mm";
    double gridSize = 10.0;
    double snapTolerance = 0.1;
    
    // View settings
    glm::vec3 backgroundColor{0.2f, 0.2f, 0.25f};
    bool showGrid = true;
    bool showAxes = true;
    bool showOrigin = true;
    
    // Timestamp
    std::string createdDate;
    std::string modifiedDate;
    
    // Application info
    std::string appVersion;
};

/**
 * Mesh data for a body (triangulated representation)
 */
struct MeshData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;
    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
};

/**
 * Native .dca format reader/writer
 * Format structure:
 * - ZIP container with:
 *   - manifest.json (metadata, settings, structure)
 *   - meshes/ (binary mesh data)
 *   - surfaces/ (NURBS surface data)
 *   - sketches/ (sketch data)
 *   - thumbnails/ (preview images)
 */
class NativeFormat {
public:
    NativeFormat();
    ~NativeFormat();
    
    /**
     * Save project to .dca file
     * @param project The project to save
     * @param filename Output file path
     * @return true if save successful
     */
    bool saveProject(const Project& project, const std::string& filename);
    
    /**
     * Load project from .dca file
     * @param filename Input file path
     * @return Loaded project, or nullptr on failure
     */
    std::shared_ptr<Project> loadProject(const std::string& filename);
    
    /**
     * Get last error message
     */
    const std::string& getErrorMessage() const { return m_errorMessage; }
    
    /**
     * Check if a file is a valid .dca file
     */
    static bool isValidDCAFile(const std::string& filename);
    
    /**
     * Get file info without fully loading
     */
    struct FileInfo {
        std::string name;
        std::string author;
        std::string description;
        uint32_t version;
        std::string createdDate;
        std::string modifiedDate;
        size_t bodyCount;
        size_t sketchCount;
    };
    
    static FileInfo getFileInfo(const std::string& filename);
    
private:
    std::string m_errorMessage;
    
    // JSON serialization
    std::string createManifestJSON(const Project& project);
    bool parseManifestJSON(const std::string& json, Project& project);
    
    // Binary data serialization
    std::vector<uint8_t> serializeMeshData(const MeshData& mesh);
    MeshData deserializeMeshData(const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serializeSurfaceData(const NURBSSurface& surface);
    std::shared_ptr<NURBSSurface> deserializeSurfaceData(const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serializeCurveData(const NURBSCurve& curve);
    std::shared_ptr<NURBSCurve> deserializeCurveData(const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serializeSketchData(const Sketch& sketch);
    std::shared_ptr<Sketch> deserializeSketchData(const std::vector<uint8_t>& data);
    
    // ZIP operations (simplified - real implementation would use minizip or similar)
    bool writeZipFile(const std::string& filename, 
                      const std::map<std::string, std::vector<uint8_t>>& contents);
    bool readZipFile(const std::string& filename,
                     std::map<std::string, std::vector<uint8_t>>& contents);
    
    // Utility
    void writeUint32(std::vector<uint8_t>& buffer, uint32_t value);
    void writeFloat(std::vector<uint8_t>& buffer, float value);
    void writeDouble(std::vector<uint8_t>& buffer, double value);
    void writeVec3(std::vector<uint8_t>& buffer, const glm::vec3& v);
    void writeDVec3(std::vector<uint8_t>& buffer, const glm::dvec3& v);
    void writeString(std::vector<uint8_t>& buffer, const std::string& str);
    
    uint32_t readUint32(const std::vector<uint8_t>& buffer, size_t& offset);
    float readFloat(const std::vector<uint8_t>& buffer, size_t& offset);
    double readDouble(const std::vector<uint8_t>& buffer, size_t& offset);
    glm::vec3 readVec3(const std::vector<uint8_t>& buffer, size_t& offset);
    glm::dvec3 readDVec3(const std::vector<uint8_t>& buffer, size_t& offset);
    std::string readString(const std::vector<uint8_t>& buffer, size_t& offset);
    
    std::string getCurrentTimestamp();
};

/**
 * Simple uncompressed archive format (alternative to ZIP)
 * For simpler implementation without external dependencies
 */
class SimpleArchive {
public:
    struct Entry {
        std::string name;
        std::vector<uint8_t> data;
    };
    
    bool write(const std::string& filename, const std::vector<Entry>& entries);
    bool read(const std::string& filename, std::vector<Entry>& entries);
    
private:
    static constexpr uint32_t ARCHIVE_MAGIC = 0x41524348;  // "ARCH"
};

} // namespace dc
