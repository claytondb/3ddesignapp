/**
 * @file TriangleMesh.h
 * @brief Triangle mesh wrapper for compatibility
 * 
 * This is a compatibility header that wraps MeshData for use in
 * freeform surface generation code.
 */

#pragma once

#include "../MeshData.h"

namespace dc {

/**
 * @brief Triangle mesh class - wrapper around MeshData for compatibility
 * 
 * Provides a compatibility layer for code that expects a TriangleMesh type.
 * Internally uses dc3d::geometry::MeshData.
 */
class TriangleMesh {
public:
    TriangleMesh() = default;
    ~TriangleMesh() = default;
    
    TriangleMesh(const TriangleMesh&) = default;
    TriangleMesh& operator=(const TriangleMesh&) = default;
    TriangleMesh(TriangleMesh&&) = default;
    TriangleMesh& operator=(TriangleMesh&&) = default;
    
    /**
     * @brief Construct from MeshData
     */
    explicit TriangleMesh(const dc3d::geometry::MeshData& meshData)
        : m_data(meshData) {}
    
    /**
     * @brief Get vertex count
     */
    size_t vertexCount() const { return m_data.vertexCount(); }
    
    /**
     * @brief Get face count
     */
    size_t faceCount() const { return m_data.faceCount(); }
    
    /**
     * @brief Check if empty
     */
    bool isEmpty() const { return m_data.isEmpty(); }
    
    /**
     * @brief Get vertices
     */
    const std::vector<glm::vec3>& vertices() const { return m_data.vertices(); }
    std::vector<glm::vec3>& vertices() { return m_data.vertices(); }
    
    /**
     * @brief Get indices
     */
    const std::vector<uint32_t>& indices() const { return m_data.indices(); }
    std::vector<uint32_t>& indices() { return m_data.indices(); }
    
    /**
     * @brief Get normals
     */
    const std::vector<glm::vec3>& normals() const { return m_data.normals(); }
    std::vector<glm::vec3>& normals() { return m_data.normals(); }
    
    /**
     * @brief Compute normals
     */
    void computeNormals() { m_data.computeNormals(); }
    
    /**
     * @brief Get face normal
     */
    glm::vec3 faceNormal(size_t faceIdx) const { return m_data.faceNormal(faceIdx); }
    
    /**
     * @brief Get underlying MeshData
     */
    dc3d::geometry::MeshData& meshData() { return m_data; }
    const dc3d::geometry::MeshData& meshData() const { return m_data; }
    
private:
    dc3d::geometry::MeshData m_data;
};

} // namespace dc
