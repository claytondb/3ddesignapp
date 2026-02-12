/**
 * @file ShaderProgram.cpp
 * @brief Implementation of shader program wrapper
 */

#include "ShaderProgram.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

namespace dc {

ShaderProgram::ShaderProgram()
    : m_program(std::make_unique<QOpenGLShaderProgram>())
{
}

ShaderProgram::~ShaderProgram()
{
    if (m_isBound && m_program) {
        m_program->release();
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_program(std::move(other.m_program))
    , m_errorLog(std::move(other.m_errorLog))
    , m_isBound(other.m_isBound)
    , m_uniformCache(std::move(other.m_uniformCache))
{
    other.m_isBound = false;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other) {
        if (m_isBound && m_program) {
            m_program->release();
        }
        m_program = std::move(other.m_program);
        m_errorLog = std::move(other.m_errorLog);
        m_isBound = other.m_isBound;
        m_uniformCache = std::move(other.m_uniformCache);
        other.m_isBound = false;
    }
    return *this;
}

bool ShaderProgram::loadFromSource(const QString& vertexSource, const QString& fragmentSource)
{
    m_errorLog.clear();
    m_uniformCache.clear();
    
    // Create new program
    m_program = std::make_unique<QOpenGLShaderProgram>();
    
    // Compile vertex shader
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        m_errorLog = "Vertex shader compilation failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    // Compile fragment shader
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        m_errorLog = "Fragment shader compilation failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    // Bind standard attribute locations before linking
    m_program->bindAttributeLocation("position", ATTR_POSITION);
    m_program->bindAttributeLocation("normal", ATTR_NORMAL);
    m_program->bindAttributeLocation("texCoord", ATTR_TEXCOORD);
    m_program->bindAttributeLocation("color", ATTR_COLOR);
    m_program->bindAttributeLocation("tangent", ATTR_TANGENT);
    
    // Link program
    if (!m_program->link()) {
        m_errorLog = "Shader program linking failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    return true;
}

bool ShaderProgram::loadFromSource(const QString& vertexSource,
                                    const QString& geometrySource,
                                    const QString& fragmentSource)
{
    m_errorLog.clear();
    m_uniformCache.clear();
    
    m_program = std::make_unique<QOpenGLShaderProgram>();
    
    // Compile vertex shader
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        m_errorLog = "Vertex shader compilation failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    // Compile geometry shader
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Geometry, geometrySource)) {
        m_errorLog = "Geometry shader compilation failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    // Compile fragment shader
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        m_errorLog = "Fragment shader compilation failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    // Bind standard attribute locations
    m_program->bindAttributeLocation("position", ATTR_POSITION);
    m_program->bindAttributeLocation("normal", ATTR_NORMAL);
    m_program->bindAttributeLocation("texCoord", ATTR_TEXCOORD);
    m_program->bindAttributeLocation("color", ATTR_COLOR);
    m_program->bindAttributeLocation("tangent", ATTR_TANGENT);
    
    // Link program
    if (!m_program->link()) {
        m_errorLog = "Shader program linking failed:\n" + m_program->log();
        qWarning() << m_errorLog;
        return false;
    }
    
    return true;
}

bool ShaderProgram::loadFromFiles(const QString& vertexPath, const QString& fragmentPath)
{
    QString vertexSource = readFile(vertexPath);
    QString fragmentSource = readFile(fragmentPath);
    
    if (vertexSource.isEmpty()) {
        m_errorLog = "Failed to read vertex shader: " + vertexPath;
        return false;
    }
    
    if (fragmentSource.isEmpty()) {
        m_errorLog = "Failed to read fragment shader: " + fragmentPath;
        return false;
    }
    
    return loadFromSource(vertexSource, fragmentSource);
}

bool ShaderProgram::loadFromFiles(const QString& vertexPath,
                                   const QString& geometryPath,
                                   const QString& fragmentPath)
{
    QString vertexSource = readFile(vertexPath);
    QString geometrySource = readFile(geometryPath);
    QString fragmentSource = readFile(fragmentPath);
    
    if (vertexSource.isEmpty() || geometrySource.isEmpty() || fragmentSource.isEmpty()) {
        m_errorLog = "Failed to read shader files";
        return false;
    }
    
    return loadFromSource(vertexSource, geometrySource, fragmentSource);
}

bool ShaderProgram::loadFromResources(const QString& vertexResource, const QString& fragmentResource)
{
    return loadFromFiles(vertexResource, fragmentResource);
}

void ShaderProgram::bind()
{
    if (m_program && m_program->isLinked()) {
        m_program->bind();
        m_isBound = true;
    }
}

void ShaderProgram::release()
{
    if (m_program && m_isBound) {
        m_program->release();
        m_isBound = false;
    }
}

bool ShaderProgram::isValid() const
{
    return m_program && m_program->isLinked();
}

GLuint ShaderProgram::programId() const
{
    return m_program ? m_program->programId() : 0;
}

int ShaderProgram::getUniformLocation(const char* name)
{
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }
    
    int location = m_program ? m_program->uniformLocation(name) : -1;
    m_uniformCache[name] = location;
    return location;
}

void ShaderProgram::setUniform(const char* name, int value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, float value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, double value)
{
    setUniform(name, static_cast<float>(value));
}

void ShaderProgram::setUniform(const char* name, bool value)
{
    setUniform(name, value ? 1 : 0);
}

void ShaderProgram::setUniform(const char* name, const QVector2D& value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, const QVector3D& value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, const QVector4D& value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, const QMatrix3x3& value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, const QMatrix4x4& value)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValue(loc, value);
    }
}

void ShaderProgram::setUniform(const char* name, const QColor& value)
{
    QVector4D colorVec(value.redF(), value.greenF(), value.blueF(), value.alphaF());
    setUniform(name, colorVec);
}

void ShaderProgram::setUniformArray(const char* name, const float* values, int count)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValueArray(loc, values, count, 1);
    }
}

void ShaderProgram::setUniformArray(const char* name, const QVector3D* values, int count)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValueArray(loc, values, count);
    }
}

void ShaderProgram::setUniformArray(const char* name, const QMatrix4x4* values, int count)
{
    int loc = getUniformLocation(name);
    if (loc >= 0 && m_program) {
        m_program->setUniformValueArray(loc, values, count);
    }
}

int ShaderProgram::attributeLocation(const char* name) const
{
    return m_program ? m_program->attributeLocation(name) : -1;
}

void ShaderProgram::bindAttributeLocation(const char* name, int location)
{
    if (m_program) {
        m_program->bindAttributeLocation(name, location);
    }
}

QString ShaderProgram::readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open shader file:" << path;
        return QString();
    }
    
    QTextStream stream(&file);
    return stream.readAll();
}

} // namespace dc
