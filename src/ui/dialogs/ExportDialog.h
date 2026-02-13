#pragma once

#include "../../io/ExportOptions.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace dc {

// Forward declarations
class Model;

/**
 * Export dialog for selecting format and options
 * Renders using ImGui
 */
class ExportDialog {
public:
    ExportDialog();
    ~ExportDialog();
    
    /**
     * Open the dialog
     * @param model The model to export (for size estimation)
     */
    void open(std::shared_ptr<Model> model);
    
    /**
     * Close the dialog
     */
    void close();
    
    /**
     * Check if dialog is open
     */
    bool isOpen() const { return m_isOpen; }
    
    /**
     * Render the dialog (call each frame)
     * @return true if export was confirmed
     */
    bool render();
    
    /**
     * Get the configured export options
     */
    const ExportOptions& getOptions() const { return m_options; }
    
    /**
     * Get the selected file path
     */
    const std::string& getFilePath() const { return m_filePath; }
    
    /**
     * Set callback for when export is confirmed
     */
    void setExportCallback(std::function<void(const std::string&, const ExportOptions&)> callback);
    
    /**
     * Set default export directory
     */
    void setDefaultDirectory(const std::string& dir) { m_defaultDirectory = dir; }
    
private:
    bool m_isOpen = false;
    ExportOptions m_options;
    std::string m_filePath;
    std::string m_defaultDirectory;
    std::shared_ptr<Model> m_model;
    
    std::function<void(const std::string&, const ExportOptions&)> m_exportCallback;
    
    // UI state
    int m_selectedFormat = 0;
    int m_selectedQuality = 1;  // Standard
    int m_selectedUnits = 0;    // Millimeters
    int m_stepVersion = 1;      // AP214
    bool m_showAdvancedOptions = false;
    
    char m_fileNameBuffer[256] = "export";
    char m_authorBuffer[128] = "";
    char m_organizationBuffer[128] = "";
    
    // Format names for combo box
    static const std::vector<std::string> s_formatNames;
    static const std::vector<std::string> s_qualityNames;
    static const std::vector<std::string> s_unitNames;
    
    // Rendering helpers
    void renderFormatSelector();
    void renderSTEPOptions();
    void renderIGESOptions();
    void renderSTLOptions();
    void renderTessellationOptions();
    void renderAdvancedOptions();
    void renderFilePathSelector();
    void renderFileSizeEstimate();
    void renderActionButtons();
    
    // Update options based on UI state
    void updateOptionsFromUI();
    
    // Estimate file size
    size_t estimateFileSize() const;
    
    // Format size for display
    std::string formatFileSize(size_t bytes) const;
    
    // Open native file dialog
    bool showSaveFileDialog();
};

/**
 * Import dialog for selecting files and options
 */
class ImportDialog {
public:
    ImportDialog();
    ~ImportDialog();
    
    void open();
    void close();
    bool isOpen() const { return m_isOpen; }
    
    /**
     * Render the dialog
     * @return true if import was confirmed
     */
    bool render();
    
    const ImportOptions& getOptions() const { return m_options; }
    const std::string& getFilePath() const { return m_filePath; }
    
    void setImportCallback(std::function<void(const std::string&, const ImportOptions&)> callback);
    void setDefaultDirectory(const std::string& dir) { m_defaultDirectory = dir; }
    
private:
    bool m_isOpen = false;
    ImportOptions m_options;
    std::string m_filePath;
    std::string m_defaultDirectory;
    
    std::function<void(const std::string&, const ImportOptions&)> m_importCallback;
    
    // UI state
    int m_selectedUnits = 0;
    int m_selectedQuality = 1;
    bool m_showAdvancedOptions = false;
    
    void renderOptions();
    void renderFileSelector();
    void renderActionButtons();
    
    bool showOpenFileDialog();
};

} // namespace dc
