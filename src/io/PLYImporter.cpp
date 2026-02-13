/**
 * @file PLYImporter.cpp
 * @brief Implementation of Stanford PLY file format importer
 */

#include "PLYImporter.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <bit>

namespace dc3d {
namespace io {

namespace {

// Maximum list size to prevent DoS attacks (CRITICAL FIX)
constexpr int64_t MAX_LIST_SIZE = 10000000;  // 10M elements max

// Trim whitespace from string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// Check if system is little endian
bool isSystemLittleEndian() {
    uint16_t test = 0x0001;
    return *reinterpret_cast<uint8_t*>(&test) == 0x01;
}

} // anonymous namespace

geometry::Result<geometry::MeshData> PLYImporter::import(
    const std::filesystem::path& path,
    const PLYImportOptions& options,
    geometry::ProgressCallback progress) {
    
    // Check file exists
    if (!std::filesystem::exists(path)) {
        return geometry::Result<geometry::MeshData>::failure(
            "File not found: " + path.string());
    }
    
    // Get file size
    auto fileSize = std::filesystem::file_size(path);
    if (fileSize == 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "File is empty: " + path.string());
    }
    
    // Open file in binary mode (we'll handle ASCII separately)
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        return geometry::Result<geometry::MeshData>::failure(
            "Failed to open file: " + path.string());
    }
    
    return importFromStream(file, options, progress);
}

geometry::Result<geometry::MeshData> PLYImporter::importFromStream(
    std::istream& stream,
    const PLYImportOptions& options,
    geometry::ProgressCallback progress) {
    
    // Parse header
    auto headerResult = parseHeader(stream);
    if (!headerResult) {
        return geometry::Result<geometry::MeshData>::failure(headerResult.error);
    }
    
    const Header& header = *headerResult.value;
    
    // Import based on format
    if (header.format == Format::ASCII) {
        return readASCII(stream, header, options, progress);
    } else {
        return readBinary(stream, header, options, progress);
    }
}

geometry::Result<geometry::MeshData> PLYImporter::importFromMemory(
    const void* data,
    size_t size,
    const PLYImportOptions& options,
    geometry::ProgressCallback progress) {
    
    if (!data || size == 0) {
        return geometry::Result<geometry::MeshData>::failure("Empty data");
    }
    
    std::string dataStr(static_cast<const char*>(data), size);
    std::istringstream stream(dataStr, std::ios::binary);
    
    return importFromStream(stream, options, progress);
}

geometry::Result<PLYImporter::Header> PLYImporter::parseHeader(std::istream& stream) {
    Header header;
    
    std::string line;
    
    // First line must be "ply"
    if (!std::getline(stream, line)) {
        return geometry::Result<Header>::failure("Failed to read PLY header");
    }
    
    if (trim(line) != "ply") {
        return geometry::Result<Header>::failure("Not a PLY file (missing 'ply' magic)");
    }
    
    Element* currentElement = nullptr;
    
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;
        
        if (keyword == "format") {
            std::string formatStr, version;
            iss >> formatStr >> version;
            
            if (formatStr == "ascii") {
                header.format = Format::ASCII;
            } else if (formatStr == "binary_little_endian") {
                header.format = Format::BinaryLittleEndian;
            } else if (formatStr == "binary_big_endian") {
                header.format = Format::BinaryBigEndian;
            } else {
                return geometry::Result<Header>::failure(
                    "Unknown PLY format: " + formatStr);
            }
        }
        else if (keyword == "element") {
            std::string name;
            size_t count;
            iss >> name >> count;
            
            header.elements.push_back({name, count, {}});
            currentElement = &header.elements.back();
        }
        else if (keyword == "property") {
            if (!currentElement) {
                return geometry::Result<Header>::failure(
                    "Property definition outside of element");
            }
            
            std::string typeOrList;
            iss >> typeOrList;
            
            Property prop;
            
            if (typeOrList == "list") {
                // List property: property list <count_type> <element_type> <name>
                prop.isList = true;
                
                std::string countTypeStr, elemTypeStr;
                iss >> countTypeStr >> elemTypeStr >> prop.name;
                
                prop.listSizeType = parseDataType(countTypeStr);
                prop.listElementType = parseDataType(elemTypeStr);
                prop.type = prop.listElementType;
                
                if (prop.listSizeType == DataType::Unknown) {
                    return geometry::Result<Header>::failure(
                        "Unknown list count type: " + countTypeStr);
                }
                if (prop.listElementType == DataType::Unknown) {
                    return geometry::Result<Header>::failure(
                        "Unknown list element type: " + elemTypeStr);
                }
            } else {
                // Scalar property: property <type> <name>
                prop.type = parseDataType(typeOrList);
                iss >> prop.name;
                
                if (prop.type == DataType::Unknown) {
                    return geometry::Result<Header>::failure(
                        "Unknown property type: " + typeOrList);
                }
            }
            
            currentElement->properties.push_back(prop);
        }
        else if (keyword == "comment" || keyword == "obj_info") {
            // Ignore comments
            continue;
        }
        else if (keyword == "end_header") {
            break;
        }
    }
    
    // Validate header
    const Element* vertexElem = header.findElement("vertex");
    if (!vertexElem) {
        return geometry::Result<Header>::failure(
            "PLY file has no vertex element");
    }
    
    return geometry::Result<Header>::success(std::move(header));
}

PLYImporter::DataType PLYImporter::parseDataType(const std::string& typeStr) {
    // Standard PLY types
    if (typeStr == "char" || typeStr == "int8") return DataType::Int8;
    if (typeStr == "uchar" || typeStr == "uint8") return DataType::UInt8;
    if (typeStr == "short" || typeStr == "int16") return DataType::Int16;
    if (typeStr == "ushort" || typeStr == "uint16") return DataType::UInt16;
    if (typeStr == "int" || typeStr == "int32") return DataType::Int32;
    if (typeStr == "uint" || typeStr == "uint32") return DataType::UInt32;
    if (typeStr == "float" || typeStr == "float32") return DataType::Float32;
    if (typeStr == "double" || typeStr == "float64") return DataType::Float64;
    
    return DataType::Unknown;
}

size_t PLYImporter::dataTypeSize(DataType type) {
    switch (type) {
        case DataType::Int8:
        case DataType::UInt8:
            return 1;
        case DataType::Int16:
        case DataType::UInt16:
            return 2;
        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Float32:
            return 4;
        case DataType::Float64:
            return 8;
        default:
            return 0;
    }
}

void PLYImporter::swapBytes(void* data, size_t size) {
    uint8_t* bytes = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < size / 2; ++i) {
        std::swap(bytes[i], bytes[size - 1 - i]);
    }
}

template<typename T>
T PLYImporter::readBinaryValue(std::istream& stream, bool swapBytes) {
    T value;
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (swapBytes && sizeof(T) > 1) {
        PLYImporter::swapBytes(&value, sizeof(T));
    }
    return value;
}

double PLYImporter::readBinaryAsDouble(std::istream& stream, DataType type, bool swap) {
    switch (type) {
        case DataType::Int8:
            return static_cast<double>(readBinaryValue<int8_t>(stream, swap));
        case DataType::UInt8:
            return static_cast<double>(readBinaryValue<uint8_t>(stream, swap));
        case DataType::Int16:
            return static_cast<double>(readBinaryValue<int16_t>(stream, swap));
        case DataType::UInt16:
            return static_cast<double>(readBinaryValue<uint16_t>(stream, swap));
        case DataType::Int32:
            return static_cast<double>(readBinaryValue<int32_t>(stream, swap));
        case DataType::UInt32:
            return static_cast<double>(readBinaryValue<uint32_t>(stream, swap));
        case DataType::Float32:
            return static_cast<double>(readBinaryValue<float>(stream, swap));
        case DataType::Float64:
            return readBinaryValue<double>(stream, swap);
        default:
            return 0.0;
    }
}

int64_t PLYImporter::readBinaryAsInt(std::istream& stream, DataType type, bool swap) {
    switch (type) {
        case DataType::Int8:
            return static_cast<int64_t>(readBinaryValue<int8_t>(stream, swap));
        case DataType::UInt8:
            return static_cast<int64_t>(readBinaryValue<uint8_t>(stream, swap));
        case DataType::Int16:
            return static_cast<int64_t>(readBinaryValue<int16_t>(stream, swap));
        case DataType::UInt16:
            return static_cast<int64_t>(readBinaryValue<uint16_t>(stream, swap));
        case DataType::Int32:
            return static_cast<int64_t>(readBinaryValue<int32_t>(stream, swap));
        case DataType::UInt32:
            return static_cast<int64_t>(readBinaryValue<uint32_t>(stream, swap));
        case DataType::Float32:
            return static_cast<int64_t>(readBinaryValue<float>(stream, swap));
        case DataType::Float64:
            return static_cast<int64_t>(readBinaryValue<double>(stream, swap));
        default:
            return 0;
    }
}

geometry::Result<geometry::MeshData> PLYImporter::readASCII(
    std::istream& stream,
    const Header& header,
    const PLYImportOptions& options,
    geometry::ProgressCallback progress) {
    
    geometry::MeshData mesh;
    
    // Find vertex and face elements
    const Element* vertexElem = header.findElement("vertex");
    const Element* faceElem = header.findElement("face");
    
    if (!vertexElem) {
        return geometry::Result<geometry::MeshData>::failure(
            "No vertex element in PLY file");
    }
    
    // Find property indices for vertex element
    int xIdx = -1, yIdx = -1, zIdx = -1;
    int nxIdx = -1, nyIdx = -1, nzIdx = -1;
    int redIdx = -1, greenIdx = -1, blueIdx = -1;
    
    for (size_t i = 0; i < vertexElem->properties.size(); ++i) {
        const auto& prop = vertexElem->properties[i];
        if (prop.name == "x") xIdx = static_cast<int>(i);
        else if (prop.name == "y") yIdx = static_cast<int>(i);
        else if (prop.name == "z") zIdx = static_cast<int>(i);
        else if (prop.name == "nx") nxIdx = static_cast<int>(i);
        else if (prop.name == "ny") nyIdx = static_cast<int>(i);
        else if (prop.name == "nz") nzIdx = static_cast<int>(i);
        else if (prop.name == "red") redIdx = static_cast<int>(i);
        else if (prop.name == "green") greenIdx = static_cast<int>(i);
        else if (prop.name == "blue") blueIdx = static_cast<int>(i);
    }
    
    if (xIdx < 0 || yIdx < 0 || zIdx < 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "PLY vertex element missing x, y, or z property");
    }
    
    bool hasNormals = (nxIdx >= 0 && nyIdx >= 0 && nzIdx >= 0);
    
    // Find face list property
    int faceListIdx = -1;
    if (faceElem) {
        for (size_t i = 0; i < faceElem->properties.size(); ++i) {
            const auto& prop = faceElem->properties[i];
            if (prop.isList && 
                (prop.name == "vertex_indices" || prop.name == "vertex_index")) {
                faceListIdx = static_cast<int>(i);
                break;
            }
        }
    }
    
    // Reserve space
    mesh.reserveVertices(vertexElem->count);
    if (faceElem) {
        mesh.reserveFaces(faceElem->count);
    }
    
    size_t totalElements = vertexElem->count + (faceElem ? faceElem->count : 0);
    bool reportProgress = progress && totalElements > options.progressThreshold;
    size_t elementsProcessed = 0;
    
    // Read all elements in order
    for (const auto& element : header.elements) {
        std::string line;
        
        if (element.name == "vertex") {
            // Read vertices
            for (size_t v = 0; v < element.count; ++v) {
                if (!std::getline(stream, line)) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Unexpected end of file reading vertex " + std::to_string(v));
                }
                
                std::istringstream iss(trim(line));
                std::vector<double> values(element.properties.size());
                
                for (size_t p = 0; p < element.properties.size(); ++p) {
                    if (!(iss >> values[p])) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Failed to parse vertex " + std::to_string(v));
                    }
                }
                
                glm::vec3 pos(
                    static_cast<float>(values[xIdx]),
                    static_cast<float>(values[yIdx]),
                    static_cast<float>(values[zIdx])
                );
                
                if (hasNormals) {
                    glm::vec3 norm(
                        static_cast<float>(values[nxIdx]),
                        static_cast<float>(values[nyIdx]),
                        static_cast<float>(values[nzIdx])
                    );
                    mesh.addVertex(pos, norm);
                } else {
                    mesh.addVertex(pos);
                }
                
                ++elementsProcessed;
                if (reportProgress && (elementsProcessed % 100000 == 0)) {
                    if (!progress(static_cast<float>(elementsProcessed) / totalElements)) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Import cancelled");
                    }
                }
            }
        }
        else if (element.name == "face" && faceListIdx >= 0) {
            // Read faces
            for (size_t f = 0; f < element.count; ++f) {
                if (!std::getline(stream, line)) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Unexpected end of file reading face " + std::to_string(f));
                }
                
                std::istringstream iss(trim(line));
                
                // Skip properties before the face list
                for (int p = 0; p < faceListIdx; ++p) {
                    double dummy;
                    iss >> dummy;
                }
                
                // Read face list
                int vertexCount;
                iss >> vertexCount;
                
                if (vertexCount < 3) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Face " + std::to_string(f) + " has fewer than 3 vertices");
                }
                
                std::vector<uint32_t> indices(vertexCount);
                for (int i = 0; i < vertexCount; ++i) {
                    iss >> indices[i];
                    
                    if (indices[i] >= mesh.vertexCount()) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Face " + std::to_string(f) + " has invalid vertex index");
                    }
                }
                
                // Triangulate (fan triangulation)
                for (int i = 1; i < vertexCount - 1; ++i) {
                    mesh.addFace(indices[0], indices[i], indices[i + 1]);
                }
                
                ++elementsProcessed;
                if (reportProgress && (elementsProcessed % 100000 == 0)) {
                    if (!progress(static_cast<float>(elementsProcessed) / totalElements)) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Import cancelled");
                    }
                }
            }
        }
        else {
            // Skip unknown elements
            for (size_t i = 0; i < element.count; ++i) {
                std::getline(stream, line);
            }
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

geometry::Result<geometry::MeshData> PLYImporter::readBinary(
    std::istream& stream,
    const Header& header,
    const PLYImportOptions& options,
    geometry::ProgressCallback progress) {
    
    geometry::MeshData mesh;
    
    // Determine if we need to swap bytes
    bool systemLittleEndian = isSystemLittleEndian();
    bool needSwap = (header.format == Format::BinaryBigEndian) == systemLittleEndian;
    
    // Find vertex and face elements
    const Element* vertexElem = header.findElement("vertex");
    const Element* faceElem = header.findElement("face");
    
    if (!vertexElem) {
        return geometry::Result<geometry::MeshData>::failure(
            "No vertex element in PLY file");
    }
    
    // Find property indices for vertex element
    int xIdx = -1, yIdx = -1, zIdx = -1;
    int nxIdx = -1, nyIdx = -1, nzIdx = -1;
    
    for (size_t i = 0; i < vertexElem->properties.size(); ++i) {
        const auto& prop = vertexElem->properties[i];
        if (prop.name == "x") xIdx = static_cast<int>(i);
        else if (prop.name == "y") yIdx = static_cast<int>(i);
        else if (prop.name == "z") zIdx = static_cast<int>(i);
        else if (prop.name == "nx") nxIdx = static_cast<int>(i);
        else if (prop.name == "ny") nyIdx = static_cast<int>(i);
        else if (prop.name == "nz") nzIdx = static_cast<int>(i);
    }
    
    if (xIdx < 0 || yIdx < 0 || zIdx < 0) {
        return geometry::Result<geometry::MeshData>::failure(
            "PLY vertex element missing x, y, or z property");
    }
    
    bool hasNormals = (nxIdx >= 0 && nyIdx >= 0 && nzIdx >= 0);
    
    // Find face list property
    int faceListIdx = -1;
    DataType faceListSizeType = DataType::Unknown;
    DataType faceListElemType = DataType::Unknown;
    
    if (faceElem) {
        for (size_t i = 0; i < faceElem->properties.size(); ++i) {
            const auto& prop = faceElem->properties[i];
            if (prop.isList && 
                (prop.name == "vertex_indices" || prop.name == "vertex_index")) {
                faceListIdx = static_cast<int>(i);
                faceListSizeType = prop.listSizeType;
                faceListElemType = prop.listElementType;
                break;
            }
        }
    }
    
    // Reserve space
    mesh.reserveVertices(vertexElem->count);
    if (faceElem) {
        mesh.reserveFaces(faceElem->count);
    }
    
    size_t totalElements = vertexElem->count + (faceElem ? faceElem->count : 0);
    bool reportProgress = progress && totalElements > options.progressThreshold;
    size_t elementsProcessed = 0;
    
    // Read all elements in order
    for (const auto& element : header.elements) {
        if (element.name == "vertex") {
            // Read vertices
            for (size_t v = 0; v < element.count; ++v) {
                std::vector<double> values(element.properties.size());
                
                for (size_t p = 0; p < element.properties.size(); ++p) {
                    const auto& prop = element.properties[p];
                    if (prop.isList) {
                        // Skip list properties in vertex (unusual)
                        int64_t listSize = readBinaryAsInt(stream, prop.listSizeType, needSwap);
                        // CRITICAL FIX: Bounds check for list size to prevent DoS
                        if (listSize < 0 || listSize > MAX_LIST_SIZE) {
                            return geometry::Result<geometry::MeshData>::failure(
                                "Invalid list size in binary PLY: " + std::to_string(listSize));
                        }
                        for (int64_t i = 0; i < listSize; ++i) {
                            readBinaryAsDouble(stream, prop.listElementType, needSwap);
                        }
                        values[p] = 0;
                    } else {
                        values[p] = readBinaryAsDouble(stream, prop.type, needSwap);
                    }
                }
                
                if (!stream) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Failed to read vertex " + std::to_string(v));
                }
                
                glm::vec3 pos(
                    static_cast<float>(values[xIdx]),
                    static_cast<float>(values[yIdx]),
                    static_cast<float>(values[zIdx])
                );
                
                if (hasNormals) {
                    glm::vec3 norm(
                        static_cast<float>(values[nxIdx]),
                        static_cast<float>(values[nyIdx]),
                        static_cast<float>(values[nzIdx])
                    );
                    mesh.addVertex(pos, norm);
                } else {
                    mesh.addVertex(pos);
                }
                
                ++elementsProcessed;
                if (reportProgress && (elementsProcessed % 100000 == 0)) {
                    if (!progress(static_cast<float>(elementsProcessed) / totalElements)) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Import cancelled");
                    }
                }
            }
        }
        else if (element.name == "face" && faceListIdx >= 0) {
            // Read faces
            for (size_t f = 0; f < element.count; ++f) {
                // Skip properties before the face list
                for (int p = 0; p < faceListIdx; ++p) {
                    const auto& prop = element.properties[p];
                    if (prop.isList) {
                        int64_t listSize = readBinaryAsInt(stream, prop.listSizeType, needSwap);
                        // CRITICAL FIX: Bounds check for list size
                        if (listSize < 0 || listSize > MAX_LIST_SIZE) {
                            return geometry::Result<geometry::MeshData>::failure(
                                "Invalid list size in binary PLY: " + std::to_string(listSize));
                        }
                        for (int64_t i = 0; i < listSize; ++i) {
                            readBinaryAsDouble(stream, prop.listElementType, needSwap);
                        }
                    } else {
                        readBinaryAsDouble(stream, prop.type, needSwap);
                    }
                }
                
                // Read face list
                int64_t vertexCount = readBinaryAsInt(stream, faceListSizeType, needSwap);
                
                // CRITICAL FIX: Bounds check for vertex count
                if (vertexCount < 3 || vertexCount > MAX_LIST_SIZE) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Face " + std::to_string(f) + " has invalid vertex count: " + std::to_string(vertexCount));
                }
                
                std::vector<uint32_t> indices(vertexCount);
                for (int64_t i = 0; i < vertexCount; ++i) {
                    indices[i] = static_cast<uint32_t>(
                        readBinaryAsInt(stream, faceListElemType, needSwap));
                    
                    if (indices[i] >= mesh.vertexCount()) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Face " + std::to_string(f) + " has invalid vertex index");
                    }
                }
                
                // Skip remaining properties
                for (size_t p = faceListIdx + 1; p < element.properties.size(); ++p) {
                    const auto& prop = element.properties[p];
                    if (prop.isList) {
                        int64_t listSize = readBinaryAsInt(stream, prop.listSizeType, needSwap);
                        // CRITICAL FIX: Bounds check for list size
                        if (listSize < 0 || listSize > MAX_LIST_SIZE) {
                            return geometry::Result<geometry::MeshData>::failure(
                                "Invalid list size in binary PLY: " + std::to_string(listSize));
                        }
                        for (int64_t i = 0; i < listSize; ++i) {
                            readBinaryAsDouble(stream, prop.listElementType, needSwap);
                        }
                    } else {
                        readBinaryAsDouble(stream, prop.type, needSwap);
                    }
                }
                
                if (!stream) {
                    return geometry::Result<geometry::MeshData>::failure(
                        "Failed to read face " + std::to_string(f));
                }
                
                // Triangulate (fan triangulation)
                for (int64_t i = 1; i < vertexCount - 1; ++i) {
                    mesh.addFace(indices[0], indices[i], indices[i + 1]);
                }
                
                ++elementsProcessed;
                if (reportProgress && (elementsProcessed % 100000 == 0)) {
                    if (!progress(static_cast<float>(elementsProcessed) / totalElements)) {
                        return geometry::Result<geometry::MeshData>::failure(
                            "Import cancelled");
                    }
                }
            }
        }
        else {
            // Skip unknown elements
            for (size_t i = 0; i < element.count; ++i) {
                for (const auto& prop : element.properties) {
                    if (prop.isList) {
                        int64_t listSize = readBinaryAsInt(stream, prop.listSizeType, needSwap);
                        // CRITICAL FIX: Bounds check for list size
                        if (listSize < 0 || listSize > MAX_LIST_SIZE) {
                            return geometry::Result<geometry::MeshData>::failure(
                                "Invalid list size in binary PLY: " + std::to_string(listSize));
                        }
                        for (int64_t j = 0; j < listSize; ++j) {
                            readBinaryAsDouble(stream, prop.listElementType, needSwap);
                        }
                    } else {
                        readBinaryAsDouble(stream, prop.type, needSwap);
                    }
                }
            }
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

} // namespace io
} // namespace dc3d
