/**
 * @file OperationResult.h
 * @brief User-friendly operation result with detailed feedback
 * 
 * Provides clear, human-readable messages for operation outcomes,
 * including success statistics, warnings, and detailed error explanations.
 */

#ifndef DC3D_CORE_OPERATIONRESULT_H
#define DC3D_CORE_OPERATIONRESULT_H

#include <QString>
#include <QStringList>
#include <optional>
#include <chrono>

namespace dc3d {
namespace core {

/**
 * @enum OperationSeverity
 * @brief Severity level for operation outcomes
 */
enum class OperationSeverity {
    Success,    ///< Operation completed successfully
    Info,       ///< Informational message
    Warning,    ///< Operation succeeded with warnings
    Error       ///< Operation failed
};

/**
 * @class OperationResult
 * @brief Comprehensive operation result with user feedback
 * 
 * Provides detailed, human-readable feedback for all operations:
 * - Clear success messages with statistics
 * - Specific error messages explaining WHY something failed
 * - Warnings for partial successes
 * - Timing information for performance feedback
 * 
 * Usage:
 * @code
 * auto result = OperationResult::success("Import complete")
 *     .withDetail("File", "model.stl")
 *     .withStatistic("Triangles", 50000)
 *     .withStatistic("Vertices", 25000)
 *     .withTiming(loadTimeMs);
 * @endcode
 */
class OperationResult
{
public:
    // Factory methods for common outcomes
    static OperationResult success(const QString& message);
    static OperationResult info(const QString& message);
    static OperationResult warning(const QString& message);
    static OperationResult error(const QString& message);
    
    // Predefined error messages for common failures
    static OperationResult fileNotFound(const QString& filePath);
    static OperationResult fileEmpty(const QString& filePath);
    static OperationResult fileCorrupted(const QString& filePath, const QString& reason = QString());
    static OperationResult unsupportedFormat(const QString& format);
    static OperationResult permissionDenied(const QString& filePath);
    static OperationResult outOfMemory(const QString& operation);
    static OperationResult invalidInput(const QString& paramName, const QString& reason);
    static OperationResult operationCancelled(const QString& operation);
    static OperationResult meshEmpty(const QString& operation);
    static OperationResult meshInvalid(const QString& operation, const QString& issue);
    static OperationResult noChangesNeeded(const QString& operation, const QString& reason);
    
    // Builder pattern for adding details
    OperationResult& withDetail(const QString& key, const QString& value);
    OperationResult& withDetail(const QString& key, int value);
    OperationResult& withDetail(const QString& key, size_t value);
    OperationResult& withDetail(const QString& key, double value, int precision = 2);
    OperationResult& withStatistic(const QString& name, size_t value);
    OperationResult& withStatistic(const QString& name, double value, int precision = 2);
    OperationResult& withStatistic(const QString& name, const QString& formattedValue);
    OperationResult& withWarning(const QString& warning);
    OperationResult& withTiming(double milliseconds);
    OperationResult& withFilePath(const QString& path);
    OperationResult& withSuggestion(const QString& suggestion);
    
    // Add before/after statistics for operations that change mesh data
    OperationResult& withBeforeAfter(const QString& metric, size_t before, size_t after);
    OperationResult& withReduction(const QString& metric, size_t original, size_t final);
    
    // Query methods
    bool isSuccess() const { return m_severity == OperationSeverity::Success || 
                                    m_severity == OperationSeverity::Info; }
    bool isWarning() const { return m_severity == OperationSeverity::Warning; }
    bool isError() const { return m_severity == OperationSeverity::Error; }
    bool hasWarnings() const { return !m_warnings.isEmpty(); }
    OperationSeverity severity() const { return m_severity; }
    
    // Message retrieval
    QString message() const { return m_message; }
    QString shortMessage() const;  ///< Brief message for status bar
    QString detailedMessage() const;  ///< Full message with all details
    QString htmlMessage() const;  ///< HTML formatted for dialogs
    QStringList warnings() const { return m_warnings; }
    
    // Statistics helpers
    QString formattedDuration() const;
    QString formattedStatistics() const;
    
private:
    OperationResult(OperationSeverity severity, const QString& message);
    
    OperationSeverity m_severity;
    QString m_message;
    QString m_filePath;
    QStringList m_details;
    QStringList m_statistics;
    QStringList m_warnings;
    QStringList m_suggestions;
    std::optional<double> m_durationMs;
};

// ============================================================================
// Import-specific result helpers
// ============================================================================

/**
 * @brief Create success result for mesh import
 */
OperationResult importSuccess(const QString& fileName, 
                              size_t triangleCount, 
                              size_t vertexCount,
                              double fileSizeMB,
                              double loadTimeMs);

/**
 * @brief Create detailed import error with troubleshooting suggestions
 */
OperationResult importError(const QString& fileName, 
                           const QString& errorType,
                           const QString& details = QString());

// ============================================================================
// Mesh operation result helpers
// ============================================================================

/**
 * @brief Create success result for mesh decimation
 */
OperationResult decimationSuccess(size_t originalFaces, 
                                  size_t finalFaces,
                                  double reductionPercent,
                                  double timeMs);

/**
 * @brief Create success result for mesh smoothing
 */
OperationResult smoothingSuccess(int iterations,
                                 size_t verticesMoved,
                                 double avgDisplacement,
                                 double timeMs);

/**
 * @brief Create success result for hole filling
 */
OperationResult holeFillSuccess(size_t holesFilled,
                               size_t holesSkipped,
                               size_t facesAdded,
                               double timeMs);

/**
 * @brief Create success result for mesh repair
 */
OperationResult repairSuccess(size_t issuesFixed,
                             size_t duplicatesRemoved,
                             size_t degenerateFacesRemoved,
                             double timeMs);

// ============================================================================
// Validation helpers
// ============================================================================

/**
 * @brief Validate numeric input range
 */
OperationResult validateRange(const QString& paramName, 
                             double value, 
                             double min, 
                             double max);

/**
 * @brief Validate that value is positive
 */
OperationResult validatePositive(const QString& paramName, double value);

/**
 * @brief Validate selection for operation
 */
OperationResult validateSelection(int selectedCount, 
                                 int minRequired,
                                 const QString& objectType);

/**
 * @brief Create confirmation message for destructive operation
 */
QString confirmDestructiveOperation(const QString& operation,
                                   int affectedCount,
                                   const QString& objectType);

} // namespace core
} // namespace dc3d

#endif // DC3D_CORE_OPERATIONRESULT_H
