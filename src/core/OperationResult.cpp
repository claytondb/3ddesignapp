/**
 * @file OperationResult.cpp
 * @brief Implementation of OperationResult for user-friendly feedback
 */

#include "OperationResult.h"

#include <QFileInfo>
#include <QLocale>

namespace dc3d {
namespace core {

// ============================================================================
// OperationResult Implementation
// ============================================================================

OperationResult::OperationResult(OperationSeverity severity, const QString& message)
    : m_severity(severity)
    , m_message(message)
{
}

OperationResult OperationResult::success(const QString& message)
{
    return OperationResult(OperationSeverity::Success, message);
}

OperationResult OperationResult::info(const QString& message)
{
    return OperationResult(OperationSeverity::Info, message);
}

OperationResult OperationResult::warning(const QString& message)
{
    return OperationResult(OperationSeverity::Warning, message);
}

OperationResult OperationResult::error(const QString& message)
{
    return OperationResult(OperationSeverity::Error, message);
}

// ============================================================================
// Predefined Error Messages
// ============================================================================

OperationResult OperationResult::fileNotFound(const QString& filePath)
{
    QFileInfo info(filePath);
    return error(QString("File not found: \"%1\"").arg(info.fileName()))
        .withDetail("Path", filePath)
        .withSuggestion("Check that the file exists and the path is correct");
}

OperationResult OperationResult::fileEmpty(const QString& filePath)
{
    QFileInfo info(filePath);
    return error(QString("File is empty: \"%1\"").arg(info.fileName()))
        .withDetail("Path", filePath)
        .withSuggestion("The file contains no data. It may be corrupted or incomplete");
}

OperationResult OperationResult::fileCorrupted(const QString& filePath, const QString& reason)
{
    QFileInfo info(filePath);
    QString msg = QString("File appears to be corrupted: \"%1\"").arg(info.fileName());
    auto result = error(msg).withDetail("Path", filePath);
    
    if (!reason.isEmpty()) {
        result.withDetail("Issue", reason);
    }
    
    return result.withSuggestion("Try re-exporting the file from the original application");
}

OperationResult OperationResult::unsupportedFormat(const QString& format)
{
    return error(QString("Unsupported file format: %1").arg(format))
        .withSuggestion("Supported formats: STL, OBJ, PLY, STEP, IGES");
}

OperationResult OperationResult::permissionDenied(const QString& filePath)
{
    QFileInfo info(filePath);
    return error(QString("Permission denied: \"%1\"").arg(info.fileName()))
        .withDetail("Path", filePath)
        .withSuggestion("Check that you have read permission for this file");
}

OperationResult OperationResult::outOfMemory(const QString& operation)
{
    return error(QString("Out of memory during: %1").arg(operation))
        .withSuggestion("Try closing other applications or working with a smaller mesh");
}

OperationResult OperationResult::invalidInput(const QString& paramName, const QString& reason)
{
    return error(QString("Invalid %1: %2").arg(paramName, reason));
}

OperationResult OperationResult::operationCancelled(const QString& operation)
{
    return info(QString("%1 was cancelled").arg(operation));
}

OperationResult OperationResult::meshEmpty(const QString& operation)
{
    return error(QString("Cannot %1: mesh is empty").arg(operation))
        .withSuggestion("Import or create a mesh first");
}

OperationResult OperationResult::meshInvalid(const QString& operation, const QString& issue)
{
    return error(QString("Cannot %1: %2").arg(operation, issue))
        .withSuggestion("Try running Mesh Repair before this operation");
}

OperationResult OperationResult::noChangesNeeded(const QString& operation, const QString& reason)
{
    return info(QString("%1: %2").arg(operation, reason));
}

// ============================================================================
// Builder Methods
// ============================================================================

OperationResult& OperationResult::withDetail(const QString& key, const QString& value)
{
    m_details.append(QString("%1: %2").arg(key, value));
    return *this;
}

OperationResult& OperationResult::withDetail(const QString& key, int value)
{
    return withDetail(key, QLocale().toString(value));
}

OperationResult& OperationResult::withDetail(const QString& key, size_t value)
{
    return withDetail(key, QLocale().toString(static_cast<qulonglong>(value)));
}

OperationResult& OperationResult::withDetail(const QString& key, double value, int precision)
{
    return withDetail(key, QString::number(value, 'f', precision));
}

OperationResult& OperationResult::withStatistic(const QString& name, size_t value)
{
    m_statistics.append(QString("%1: %2").arg(name, QLocale().toString(static_cast<qulonglong>(value))));
    return *this;
}

OperationResult& OperationResult::withStatistic(const QString& name, double value, int precision)
{
    m_statistics.append(QString("%1: %2").arg(name, QString::number(value, 'f', precision)));
    return *this;
}

OperationResult& OperationResult::withStatistic(const QString& name, const QString& formattedValue)
{
    m_statistics.append(QString("%1: %2").arg(name, formattedValue));
    return *this;
}

OperationResult& OperationResult::withWarning(const QString& warning)
{
    m_warnings.append(warning);
    if (m_severity == OperationSeverity::Success) {
        m_severity = OperationSeverity::Warning;
    }
    return *this;
}

OperationResult& OperationResult::withTiming(double milliseconds)
{
    m_durationMs = milliseconds;
    return *this;
}

OperationResult& OperationResult::withFilePath(const QString& path)
{
    m_filePath = path;
    return *this;
}

OperationResult& OperationResult::withSuggestion(const QString& suggestion)
{
    m_suggestions.append(suggestion);
    return *this;
}

OperationResult& OperationResult::withBeforeAfter(const QString& metric, size_t before, size_t after)
{
    QLocale locale;
    QString change;
    if (after > before) {
        change = QString("+%1").arg(locale.toString(static_cast<qulonglong>(after - before)));
    } else if (after < before) {
        change = QString("-%1").arg(locale.toString(static_cast<qulonglong>(before - after)));
    } else {
        change = "no change";
    }
    
    m_statistics.append(QString("%1: %2 → %3 (%4)")
        .arg(metric)
        .arg(locale.toString(static_cast<qulonglong>(before)))
        .arg(locale.toString(static_cast<qulonglong>(after)))
        .arg(change));
    return *this;
}

OperationResult& OperationResult::withReduction(const QString& metric, size_t original, size_t final)
{
    QLocale locale;
    
    // Guard against division by zero
    if (original == 0) {
        m_statistics.append(QString("%1: %2 → %3")
            .arg(metric)
            .arg(locale.toString(static_cast<qulonglong>(original)))
            .arg(locale.toString(static_cast<qulonglong>(final))));
        return *this;
    }
    
    double percent = 100.0 * (1.0 - static_cast<double>(final) / static_cast<double>(original));
    
    m_statistics.append(QString("%1: %2 → %3 (%4% reduction)")
        .arg(metric)
        .arg(locale.toString(static_cast<qulonglong>(original)))
        .arg(locale.toString(static_cast<qulonglong>(final)))
        .arg(QString::number(percent, 'f', 1)));
    return *this;
}

// ============================================================================
// Message Formatting
// ============================================================================

QString OperationResult::shortMessage() const
{
    // Brief message suitable for status bar
    if (m_durationMs.has_value() && m_durationMs.value() > 100) {
        return QString("%1 (%2)").arg(m_message, formattedDuration());
    }
    return m_message;
}

QString OperationResult::detailedMessage() const
{
    QStringList parts;
    parts << m_message;
    
    if (!m_statistics.isEmpty()) {
        parts << "";
        parts << m_statistics;
    }
    
    if (!m_details.isEmpty()) {
        parts << "";
        parts << m_details;
    }
    
    if (!m_warnings.isEmpty()) {
        parts << "";
        parts << "Warnings:";
        for (const QString& w : m_warnings) {
            parts << QString("  • %1").arg(w);
        }
    }
    
    if (!m_suggestions.isEmpty()) {
        parts << "";
        for (const QString& s : m_suggestions) {
            parts << QString("💡 %1").arg(s);
        }
    }
    
    if (m_durationMs.has_value()) {
        parts << "";
        parts << QString("Time: %1").arg(formattedDuration());
    }
    
    return parts.join("\n");
}

QString OperationResult::htmlMessage() const
{
    QStringList html;
    
    // Header with icon
    QString icon;
    QString color;
    switch (m_severity) {
        case OperationSeverity::Success:
            icon = "✓";
            color = "#4caf50";
            break;
        case OperationSeverity::Info:
            icon = "ℹ";
            color = "#2196f3";
            break;
        case OperationSeverity::Warning:
            icon = "⚠";
            color = "#ff9800";
            break;
        case OperationSeverity::Error:
            icon = "✗";
            color = "#f44336";
            break;
    }
    
    html << QString("<h3 style='color: %1'>%2 %3</h3>").arg(color, icon, m_message.toHtmlEscaped());
    
    if (!m_statistics.isEmpty()) {
        html << "<p><b>Statistics:</b></p><ul>";
        for (const QString& s : m_statistics) {
            html << QString("<li>%1</li>").arg(s.toHtmlEscaped());
        }
        html << "</ul>";
    }
    
    if (!m_details.isEmpty()) {
        html << "<p><b>Details:</b></p><ul>";
        for (const QString& d : m_details) {
            html << QString("<li>%1</li>").arg(d.toHtmlEscaped());
        }
        html << "</ul>";
    }
    
    if (!m_warnings.isEmpty()) {
        html << "<p style='color: #ff9800'><b>⚠ Warnings:</b></p><ul>";
        for (const QString& w : m_warnings) {
            html << QString("<li style='color: #ff9800'>%1</li>").arg(w.toHtmlEscaped());
        }
        html << "</ul>";
    }
    
    if (!m_suggestions.isEmpty()) {
        html << "<p><b>💡 Suggestions:</b></p><ul>";
        for (const QString& s : m_suggestions) {
            html << QString("<li>%1</li>").arg(s.toHtmlEscaped());
        }
        html << "</ul>";
    }
    
    if (m_durationMs.has_value()) {
        html << QString("<p><i>Time: %1</i></p>").arg(formattedDuration());
    }
    
    return html.join("\n");
}

QString OperationResult::formattedDuration() const
{
    if (!m_durationMs.has_value()) {
        return QString();
    }
    
    double ms = m_durationMs.value();
    
    if (ms < 1.0) {
        return QString("< 1 ms");
    } else if (ms < 1000.0) {
        return QString("%1 ms").arg(static_cast<int>(ms));
    } else if (ms < 60000.0) {
        return QString("%1 s").arg(ms / 1000.0, 0, 'f', 1);
    } else {
        int minutes = static_cast<int>(ms / 60000.0);
        int seconds = static_cast<int>((ms - minutes * 60000.0) / 1000.0);
        return QString("%1 min %2 s").arg(minutes).arg(seconds);
    }
}

QString OperationResult::formattedStatistics() const
{
    return m_statistics.join(" | ");
}

// ============================================================================
// Import Helpers
// ============================================================================

OperationResult importSuccess(const QString& fileName, 
                              size_t triangleCount, 
                              size_t vertexCount,
                              double fileSizeMB,
                              double loadTimeMs)
{
    return OperationResult::success(QString("Imported \"%1\"").arg(fileName))
        .withStatistic("Triangles", triangleCount)
        .withStatistic("Vertices", vertexCount)
        .withStatistic("File size", QString("%1 MB").arg(QString::number(fileSizeMB, 'f', 2)))
        .withTiming(loadTimeMs);
}

OperationResult importError(const QString& fileName, 
                           const QString& errorType,
                           const QString& details)
{
    auto result = OperationResult::error(QString("Failed to import \"%1\"").arg(fileName))
        .withDetail("Error", errorType);
    
    if (!details.isEmpty()) {
        result.withDetail("Details", details);
    }
    
    // Add suggestions based on error type
    if (errorType.contains("format", Qt::CaseInsensitive) ||
        errorType.contains("parse", Qt::CaseInsensitive)) {
        result.withSuggestion("The file may be corrupted. Try re-exporting from the original application");
    } else if (errorType.contains("memory", Qt::CaseInsensitive)) {
        result.withSuggestion("The file may be too large. Try decimating or splitting the mesh");
    } else if (errorType.contains("permission", Qt::CaseInsensitive)) {
        result.withSuggestion("Check that you have read access to this file");
    }
    
    return result;
}

// ============================================================================
// Mesh Operation Helpers
// ============================================================================

OperationResult decimationSuccess(size_t originalFaces, 
                                  size_t finalFaces,
                                  double reductionPercent,
                                  double timeMs)
{
    return OperationResult::success("Polygon reduction complete")
        .withReduction("Triangles", originalFaces, finalFaces)
        .withTiming(timeMs);
}

OperationResult smoothingSuccess(int iterations,
                                 size_t verticesMoved,
                                 double avgDisplacement,
                                 double timeMs)
{
    auto result = OperationResult::success(QString("Smoothing complete (%1 iterations)").arg(iterations));
    
    if (verticesMoved > 0) {
        result.withStatistic("Vertices modified", verticesMoved)
              .withStatistic("Average displacement", QString("%1 mm").arg(QString::number(avgDisplacement, 'f', 4)));
    } else {
        result.withDetail("Note", "No vertices were moved (mesh may already be smooth)");
    }
    
    return result.withTiming(timeMs);
}

OperationResult holeFillSuccess(size_t holesFilled,
                               size_t holesSkipped,
                               size_t facesAdded,
                               double timeMs)
{
    auto result = OperationResult::success(
        QString("Filled %1 hole%2").arg(holesFilled).arg(holesFilled != 1 ? "s" : ""));
    
    result.withStatistic("Faces added", facesAdded);
    
    if (holesSkipped > 0) {
        result.withWarning(QString("%1 hole%2 skipped (too large)")
            .arg(holesSkipped)
            .arg(holesSkipped != 1 ? "s were" : " was"));
    }
    
    return result.withTiming(timeMs);
}

OperationResult repairSuccess(size_t issuesFixed,
                             size_t duplicatesRemoved,
                             size_t degenerateFacesRemoved,
                             double timeMs)
{
    if (issuesFixed == 0 && duplicatesRemoved == 0 && degenerateFacesRemoved == 0) {
        return OperationResult::info("Mesh repair complete: no issues found")
            .withTiming(timeMs);
    }
    
    auto result = OperationResult::success(QString("Mesh repair complete: %1 issue%2 fixed")
        .arg(issuesFixed + duplicatesRemoved + degenerateFacesRemoved)
        .arg((issuesFixed + duplicatesRemoved + degenerateFacesRemoved) != 1 ? "s" : ""));
    
    if (duplicatesRemoved > 0) {
        result.withStatistic("Duplicate vertices removed", duplicatesRemoved);
    }
    if (degenerateFacesRemoved > 0) {
        result.withStatistic("Degenerate faces removed", degenerateFacesRemoved);
    }
    if (issuesFixed > 0) {
        result.withStatistic("Other issues fixed", issuesFixed);
    }
    
    return result.withTiming(timeMs);
}

// ============================================================================
// Validation Helpers
// ============================================================================

OperationResult validateRange(const QString& paramName, 
                             double value, 
                             double min, 
                             double max)
{
    if (value < min || value > max) {
        return OperationResult::error(QString("Invalid %1").arg(paramName))
            .withDetail("Value", value)
            .withDetail("Valid range", QString("%1 to %2").arg(min).arg(max));
    }
    return OperationResult::success("Valid");
}

OperationResult validatePositive(const QString& paramName, double value)
{
    if (value <= 0) {
        return OperationResult::error(QString("%1 must be greater than zero").arg(paramName))
            .withDetail("Value", value);
    }
    return OperationResult::success("Valid");
}

OperationResult validateSelection(int selectedCount, 
                                 int minRequired,
                                 const QString& objectType)
{
    if (selectedCount < minRequired) {
        if (selectedCount == 0) {
            return OperationResult::error(QString("No %1 selected").arg(objectType))
                .withSuggestion(QString("Select at least %1 %2 to continue")
                    .arg(minRequired)
                    .arg(objectType));
        } else {
            return OperationResult::error(
                QString("Not enough %1 selected (%2 of %3 required)")
                    .arg(objectType)
                    .arg(selectedCount)
                    .arg(minRequired));
        }
    }
    return OperationResult::success("Valid");
}

QString confirmDestructiveOperation(const QString& operation,
                                   int affectedCount,
                                   const QString& objectType)
{
    if (affectedCount == 1) {
        return QString("This will %1 1 %2. Continue?")
            .arg(operation.toLower(), objectType);
    } else {
        return QString("This will %1 %2 %3s. Continue?")
            .arg(operation.toLower())
            .arg(affectedCount)
            .arg(objectType);
    }
}

} // namespace core
} // namespace dc3d
