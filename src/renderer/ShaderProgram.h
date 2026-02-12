/**
 * @file ShaderProgram.h
 * @brief OpenGL shader program management
 * 
 * Provides convenient wrapper for loading, compiling, and using GLSL shaders
 * with type-safe uniform setters.
 */

#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_1_Core>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QColor>
#include <QString>
#include <memory>
#include <unordered_map>

namespace dc {

/**
 * @brief OpenGL shader program wrapper
 * 
 * Handles shader compilation, linking, and uniform management with caching.
 */
class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();
    
    // Non-copyable
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    
    // Movable
    ShaderProgram(ShaderProgram&&) noexcept;
    ShaderProgram& operator=(ShaderProgram&&) noexcept;
    
    // ---- Shader Loading ----
    
    /**
     * @brief Load shaders from source strings
     * @param vertexSource GLSL vertex shader source
     * @param fragmentSource GLSL fragment shader source
     * @return true on success, false on compilation/linking error
     */
    bool loadFromSource(const QString& vertexSource, const QString& fragmentSource);
    
    /**
     * @brief Load shaders from source with geometry shader
     * @param vertexSource GLSL vertex shader source
     * @param geometrySource GLSL geometry shader source
     * @param fragmentSource GLSL fragment shader source
     * @return true on success
     */
    bool loadFromSource(const QString& vertexSource, 
                        const QString& geometrySource,
                        const QString& fragmentSource);
    
    /**
     * @brief Load shaders from files
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @return true on success
     */
    bool loadFromFiles(const QString& vertexPath, const QString& fragmentPath);
    
    /**
     * @brief Load shaders from files with geometry shader
     * @param vertexPath Path to vertex shader file
     * @param geometryPath Path to geometry shader file
     * @param fragmentPath Path to fragment shader file
     * @return true on success
     */
    bool loadFromFiles(const QString& vertexPath,
                       const QString& geometryPath,
                       const QString& fragmentPath);
    
    /**
     * @brief Load shaders from Qt resource paths
     * @param vertexResource Resource path (e.g., ":/shaders/mesh.vert")
     * @param fragmentResource Resource path
     * @return true on success
     */
    bool loadFromResources(const QString& vertexResource, const QString& fragmentResource);
    
    // ---- Program State ----
    
    /**
     * @brief Bind shader program for use
     */
    void bind();
    
    /**
     * @brief Release shader program
     */
    void release();
    
    /**
     * @brief Check if program is currently bound
     */
    bool isBound() const { return m_isBound; }
    
    /**
     * @brief Check if program is valid (compiled and linked)
     */
    bool isValid() const;
    
    /**
     * @brief Get OpenGL program ID
     */
    GLuint programId() const;
    
    /**
     * @brief Get compilation/linking error log
     */
    QString errorLog() const { return m_errorLog; }
    
    // ---- Uniform Setters ----
    
    void setUniform(const char* name, int value);
    void setUniform(const char* name, float value);
    void setUniform(const char* name, double value);
    void setUniform(const char* name, bool value);
    
    void setUniform(const char* name, const QVector2D& value);
    void setUniform(const char* name, const QVector3D& value);
    void setUniform(const char* name, const QVector4D& value);
    
    void setUniform(const char* name, const QMatrix3x3& value);
    void setUniform(const char* name, const QMatrix4x4& value);
    
    void setUniform(const char* name, const QColor& value);
    
    // Array versions
    void setUniformArray(const char* name, const float* values, int count);
    void setUniformArray(const char* name, const QVector3D* values, int count);
    void setUniformArray(const char* name, const QMatrix4x4* values, int count);
    
    // ---- Attribute Locations ----
    
    /**
     * @brief Get attribute location by name
     * @param name Attribute name in shader
     * @return Location index, or -1 if not found
     */
    int attributeLocation(const char* name) const;
    
    /**
     * @brief Bind an attribute to a specific location
     * @param name Attribute name
     * @param location Desired location
     * @note Must be called before linking (before loadFrom*)
     */
    void bindAttributeLocation(const char* name, int location);
    
    // ---- Standard Attribute Locations ----
    
    static constexpr int ATTR_POSITION = 0;
    static constexpr int ATTR_NORMAL = 1;
    static constexpr int ATTR_TEXCOORD = 2;
    static constexpr int ATTR_COLOR = 3;
    static constexpr int ATTR_TANGENT = 4;

private:
    int getUniformLocation(const char* name);
    QString readFile(const QString& path);
    
    std::unique_ptr<QOpenGLShaderProgram> m_program;
    QString m_errorLog;
    bool m_isBound = false;
    
    // Uniform location cache
    mutable std::unordered_map<std::string, int> m_uniformCache;
};

} // namespace dc
