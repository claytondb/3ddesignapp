#include "ExportDialog.h"
#include <imgui.h>
#include <sstream>
#include <algorithm>

// Platform-specific file dialogs (simplified)
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#endif

namespace dc {

// Placeholder Model class
class Model {
public:
    std::string name = "Model";
    size_t getEstimatedVertexCount() const { return 10000; }
    size_t getEstimatedFaceCount() const { return 5000; }
    size_t getBodyCount() const { return 1; }
};

// Static format names
const std::vector<std::string> ExportDialog::s_formatNames = {
    "STEP AP203 (Geometry Only)",
    "STEP AP214 (With Colors)",
    "IGES",
    "STL (ASCII)",
    "STL (Binary)",
    "OBJ",
    "DC Design (*.dca)"
};

const std::vector<std::string> ExportDialog::s_qualityNames = {
    "Draft (Fast)",
    "Standard",
    "Fine (High Quality)",
    "Custom"
};

const std::vector<std::string> ExportDialog::s_unitNames = {
    "Millimeters (mm)",
    "Centimeters (cm)",
    "Meters (m)",
    "Inches (in)",
    "Feet (ft)"
};

ExportDialog::ExportDialog()
{
    m_defaultDirectory = "";
}

ExportDialog::~ExportDialog()
{
}

void ExportDialog::open(std::shared_ptr<Model> model)
{
    m_model = model;
    m_isOpen = true;
    
    // Reset to defaults
    m_selectedFormat = 1;  // STEP AP214
    m_selectedQuality = 1;  // Standard
    m_selectedUnits = 0;    // mm
    m_showAdvancedOptions = false;
    
    // Set default filename from model
    if (model) {
        strncpy(m_fileNameBuffer, model->name.c_str(), sizeof(m_fileNameBuffer) - 1);
    }
    
    updateOptionsFromUI();
}

void ExportDialog::close()
{
    m_isOpen = false;
}

bool ExportDialog::render()
{
    if (!m_isOpen) return false;
    
    bool exportConfirmed = false;
    
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(
        ImGui::GetIO().DisplaySize.x * 0.5f - 250,
        ImGui::GetIO().DisplaySize.y * 0.5f - 300
    ), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Export Model", &m_isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        // Format selection
        renderFormatSelector();
        
        ImGui::Separator();
        
        // Format-specific options
        switch (m_selectedFormat) {
            case 0:  // STEP AP203
            case 1:  // STEP AP214
                renderSTEPOptions();
                break;
            case 2:  // IGES
                renderIGESOptions();
                break;
            case 3:  // STL ASCII
            case 4:  // STL Binary
                renderSTLOptions();
                break;
        }
        
        // Tessellation quality (for mesh-based exports)
        if (m_selectedFormat >= 3 && m_selectedFormat <= 5) {
            ImGui::Separator();
            renderTessellationOptions();
        }
        
        // Advanced options
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Advanced Options")) {
            renderAdvancedOptions();
        }
        
        ImGui::Separator();
        
        // File path
        renderFilePathSelector();
        
        // File size estimate
        renderFileSizeEstimate();
        
        ImGui::Separator();
        
        // Action buttons
        renderActionButtons();
        
        // Check if export was clicked
        if (ImGui::IsItemClicked()) {
            exportConfirmed = true;
        }
    }
    ImGui::End();
    
    return exportConfirmed;
}

void ExportDialog::setExportCallback(std::function<void(const std::string&, const ExportOptions&)> callback)
{
    m_exportCallback = callback;
}

void ExportDialog::renderFormatSelector()
{
    ImGui::Text("Export Format:");
    ImGui::Spacing();
    
    for (size_t i = 0; i < s_formatNames.size(); i++) {
        if (ImGui::RadioButton(s_formatNames[i].c_str(), m_selectedFormat == (int)i)) {
            m_selectedFormat = i;
            updateOptionsFromUI();
        }
    }
}

void ExportDialog::renderSTEPOptions()
{
    ImGui::Text("STEP Options:");
    ImGui::Spacing();
    
    // STEP version
    ImGui::Text("Application Protocol:");
    if (ImGui::RadioButton("AP203 (Geometry Only)", m_stepVersion == 0)) {
        m_stepVersion = 0;
        m_selectedFormat = 0;
        updateOptionsFromUI();
    }
    if (ImGui::RadioButton("AP214 (With Colors/Layers)", m_stepVersion == 1)) {
        m_stepVersion = 1;
        m_selectedFormat = 1;
        updateOptionsFromUI();
    }
    
    ImGui::Spacing();
    
    // Color options (AP214 only)
    if (m_stepVersion == 1) {
        ImGui::Checkbox("Include Colors", &m_options.includeColors);
        ImGui::Checkbox("Include Layer Info", &m_options.includeLayerInfo);
    }
    
    ImGui::Checkbox("Export as Assembly", &m_options.exportAsAssembly);
    
    ImGui::Spacing();
    
    // Author info
    ImGui::Text("Author Information:");
    ImGui::InputText("Author", m_authorBuffer, sizeof(m_authorBuffer));
    ImGui::InputText("Organization", m_organizationBuffer, sizeof(m_organizationBuffer));
    
    m_options.authorName = m_authorBuffer;
    m_options.organizationName = m_organizationBuffer;
}

void ExportDialog::renderIGESOptions()
{
    ImGui::Text("IGES Options:");
    ImGui::Spacing();
    
    // IGES version
    static const char* igesVersions[] = { "5.1 (9)", "5.2 (10)", "5.3 (11)" };
    int igesVersionIndex = m_options.igesVersion - 9;
    if (igesVersionIndex < 0) igesVersionIndex = 2;
    if (ImGui::Combo("IGES Version", &igesVersionIndex, igesVersions, 3)) {
        m_options.igesVersion = igesVersionIndex + 9;
    }
    
    ImGui::Checkbox("Include Colors", &m_options.igesIncludeColors);
    
    ImGui::Spacing();
    ImGui::InputText("Author", m_authorBuffer, sizeof(m_authorBuffer));
    ImGui::InputText("Organization", m_organizationBuffer, sizeof(m_organizationBuffer));
    
    m_options.authorName = m_authorBuffer;
    m_options.organizationName = m_organizationBuffer;
}

void ExportDialog::renderSTLOptions()
{
    ImGui::Text("STL Options:");
    ImGui::Spacing();
    
    // Binary vs ASCII
    bool isBinary = (m_selectedFormat == 4);
    if (ImGui::Checkbox("Binary Format", &isBinary)) {
        m_selectedFormat = isBinary ? 4 : 3;
        m_options.stlBinary = isBinary;
    }
    
    ImGui::Checkbox("Include Normals", &m_options.stlIncludeNormals);
    
    if (!isBinary) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), 
            "Note: ASCII format creates larger files");
    }
}

void ExportDialog::renderTessellationOptions()
{
    ImGui::Text("Tessellation Quality:");
    ImGui::Spacing();
    
    for (size_t i = 0; i < s_qualityNames.size(); i++) {
        if (ImGui::RadioButton(s_qualityNames[i].c_str(), m_selectedQuality == (int)i)) {
            m_selectedQuality = i;
            m_options.applyQualityPreset(static_cast<TessellationQuality>(i));
        }
    }
    
    // Custom settings
    if (m_selectedQuality == 3) {
        ImGui::Indent();
        ImGui::SliderDouble("Chord Tolerance", &m_options.chordTolerance, 0.001, 1.0, "%.3f");
        ImGui::SliderDouble("Angle Tolerance", &m_options.angleTolerance, 1.0, 45.0, "%.1f°");
        ImGui::SliderDouble("Min Edge Length", &m_options.minEdgeLength, 0.0001, 1.0, "%.4f");
        ImGui::SliderDouble("Max Edge Length", &m_options.maxEdgeLength, 1.0, 1000.0, "%.1f");
        ImGui::Unindent();
    }
}

void ExportDialog::renderAdvancedOptions()
{
    // Units
    ImGui::Text("Units:");
    if (ImGui::Combo("##units", &m_selectedUnits, 
        "Millimeters\0Centimeters\0Meters\0Inches\0Feet\0")) {
        m_options.units = static_cast<ExportUnits>(m_selectedUnits);
    }
    
    ImGui::Spacing();
    
    // Coordinate system
    ImGui::Text("Coordinate System:");
    static int coordSystem = 1;  // Z-up default
    if (ImGui::RadioButton("Y-Up (OpenGL)", coordSystem == 0)) {
        coordSystem = 0;
        m_options.coordSystem = CoordinateSystem::RightHanded_YUp;
    }
    if (ImGui::RadioButton("Z-Up (CAD Standard)", coordSystem == 1)) {
        coordSystem = 1;
        m_options.coordSystem = CoordinateSystem::RightHanded_ZUp;
    }
    
    ImGui::Spacing();
    
    // Geometry options
    ImGui::Checkbox("Export Hidden Objects", &m_options.exportHiddenObjects);
    ImGui::Checkbox("Merge Coplanar Faces", &m_options.mergeCoplanarFaces);
    ImGui::Checkbox("Heal Geometry", &m_options.healGeometry);
    
    ImGui::Spacing();
    
    // Scale factor
    ImGui::InputDouble("Scale Factor", &m_options.scaleFactor, 0.1, 1.0, "%.4f");
}

void ExportDialog::renderFilePathSelector()
{
    ImGui::Text("File Name:");
    
    ImGui::InputText("##filename", m_fileNameBuffer, sizeof(m_fileNameBuffer));
    
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        if (showSaveFileDialog()) {
            // Extract filename from path
            std::string path = m_filePath;
            size_t lastSlash = path.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                std::string name = path.substr(lastSlash + 1);
                size_t lastDot = name.find_last_of('.');
                if (lastDot != std::string::npos) {
                    name = name.substr(0, lastDot);
                }
                strncpy(m_fileNameBuffer, name.c_str(), sizeof(m_fileNameBuffer) - 1);
            }
        }
    }
    
    // Show full path
    if (!m_filePath.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Path: %s", m_filePath.c_str());
    }
}

void ExportDialog::renderFileSizeEstimate()
{
    ImGui::Spacing();
    
    size_t estimatedSize = estimateFileSize();
    std::string sizeStr = formatFileSize(estimatedSize);
    
    ImGui::Text("Estimated file size: %s", sizeStr.c_str());
    
    // Warning for large files
    if (estimatedSize > 100 * 1024 * 1024) {  // > 100 MB
        ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), 
            "Warning: Large file size. Consider using binary format or reducing quality.");
    }
}

void ExportDialog::renderActionButtons()
{
    float buttonWidth = 120;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float totalWidth = buttonWidth * 2 + spacing;
    
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);
    
    if (ImGui::Button("Export", ImVec2(buttonWidth, 0))) {
        // Build file path
        std::string extension = m_options.getFileExtension();
        m_filePath = m_defaultDirectory;
        if (!m_filePath.empty() && m_filePath.back() != '/' && m_filePath.back() != '\\') {
            m_filePath += "/";
        }
        m_filePath += m_fileNameBuffer;
        m_filePath += extension;
        
        // Call callback
        if (m_exportCallback) {
            m_exportCallback(m_filePath, m_options);
        }
        
        close();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
        close();
    }
}

void ExportDialog::updateOptionsFromUI()
{
    // Update format
    switch (m_selectedFormat) {
        case 0: m_options.format = ExportFormat::STEP_AP203; break;
        case 1: m_options.format = ExportFormat::STEP_AP214; break;
        case 2: m_options.format = ExportFormat::IGES; break;
        case 3: m_options.format = ExportFormat::STL_ASCII; break;
        case 4: m_options.format = ExportFormat::STL_BINARY; break;
        case 5: m_options.format = ExportFormat::OBJ; break;
        case 6: m_options.format = ExportFormat::NATIVE_DCA; break;
    }
    
    // Update quality preset
    m_options.applyQualityPreset(static_cast<TessellationQuality>(m_selectedQuality));
    
    // Update units
    m_options.units = static_cast<ExportUnits>(m_selectedUnits);
}

size_t ExportDialog::estimateFileSize() const
{
    if (!m_model) return 0;
    
    size_t vertexCount = m_model->getEstimatedVertexCount();
    size_t faceCount = m_model->getEstimatedFaceCount();
    
    // Rough estimates based on format
    switch (m_options.format) {
        case ExportFormat::STEP_AP203:
        case ExportFormat::STEP_AP214:
            // STEP: ~200 bytes per vertex, ~500 bytes per face
            return vertexCount * 200 + faceCount * 500 + 5000;
            
        case ExportFormat::IGES:
            // IGES: ~150 bytes per vertex, ~400 bytes per face
            return vertexCount * 150 + faceCount * 400 + 3000;
            
        case ExportFormat::STL_BINARY:
            // Binary STL: 84 bytes header + 50 bytes per triangle
            return 84 + faceCount * 50;
            
        case ExportFormat::STL_ASCII:
            // ASCII STL: ~200 bytes per triangle
            return faceCount * 200;
            
        case ExportFormat::OBJ:
            // OBJ: ~60 bytes per vertex, ~30 bytes per face
            return vertexCount * 60 + faceCount * 30;
            
        case ExportFormat::NATIVE_DCA:
            // Native: binary mesh + JSON metadata
            return vertexCount * 32 + faceCount * 12 + 10000;
            
        default:
            return 0;
    }
}

std::string ExportDialog::formatFileSize(size_t bytes) const
{
    const char* units[] = { "B", "KB", "MB", "GB" };
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }
    
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unitIndex];
    return ss.str();
}

bool ExportDialog::showSaveFileDialog()
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    
    std::string filter;
    std::string ext = m_options.getFileExtension();
    std::string formatName = m_options.getFormatName();
    
    filter = formatName + " (*" + ext + ")" + '\0' + "*" + ext + '\0';
    filter += "All Files (*.*)" + '\0' + "*.*" + '\0';
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = m_defaultDirectory.empty() ? NULL : m_defaultDirectory.c_str();
    ofn.lpstrDefExt = ext.c_str() + 1;  // Skip the dot
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameA(&ofn)) {
        m_filePath = szFile;
        return true;
    }
#endif
    
    // Fallback: just use current directory
    m_filePath = m_defaultDirectory + "/" + m_fileNameBuffer + m_options.getFileExtension();
    return true;
}

// ImportDialog implementation
ImportDialog::ImportDialog()
{
}

ImportDialog::~ImportDialog()
{
}

void ImportDialog::open()
{
    m_isOpen = true;
    m_selectedUnits = 0;
    m_selectedQuality = 1;
    m_showAdvancedOptions = false;
}

void ImportDialog::close()
{
    m_isOpen = false;
}

bool ImportDialog::render()
{
    if (!m_isOpen) return false;
    
    bool importConfirmed = false;
    
    ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Import File", &m_isOpen, ImGuiWindowFlags_NoCollapse)) {
        
        // File selector
        renderFileSelector();
        
        ImGui::Separator();
        
        // Options
        renderOptions();
        
        ImGui::Separator();
        
        // Action buttons
        renderActionButtons();
    }
    ImGui::End();
    
    return importConfirmed;
}

void ImportDialog::setImportCallback(std::function<void(const std::string&, const ImportOptions&)> callback)
{
    m_importCallback = callback;
}

void ImportDialog::renderFileSelector()
{
    ImGui::Text("Select File to Import:");
    ImGui::Spacing();
    
    static char fileBuffer[512] = "";
    ImGui::InputText("##filepath", fileBuffer, sizeof(fileBuffer));
    
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        if (showOpenFileDialog()) {
            strncpy(fileBuffer, m_filePath.c_str(), sizeof(fileBuffer) - 1);
        }
    }
    
    m_filePath = fileBuffer;
    
    // Show detected format
    if (!m_filePath.empty()) {
        std::string ext = m_filePath.substr(m_filePath.find_last_of('.'));
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        std::string format;
        if (ext == ".step" || ext == ".stp") format = "STEP";
        else if (ext == ".iges" || ext == ".igs") format = "IGES";
        else if (ext == ".stl") format = "STL";
        else if (ext == ".obj") format = "OBJ";
        else if (ext == ".dca") format = "DC Design";
        else format = "Unknown";
        
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "Detected format: %s", format.c_str());
    }
}

void ImportDialog::renderOptions()
{
    ImGui::Text("Import Options:");
    ImGui::Spacing();
    
    // Units assumption
    ImGui::Text("Assume units (if not specified):");
    ImGui::Combo("##units", &m_selectedUnits, 
        "Millimeters\0Centimeters\0Meters\0Inches\0Feet\0");
    m_options.assumedUnits = static_cast<ExportUnits>(m_selectedUnits);
    
    ImGui::Spacing();
    
    // Tessellation quality
    ImGui::Text("Tessellation Quality:");
    static const char* qualityItems[] = { "Draft", "Standard", "Fine" };
    ImGui::Combo("##quality", &m_selectedQuality, qualityItems, 3);
    m_options.tessQuality = static_cast<TessellationQuality>(m_selectedQuality);
    
    ImGui::Spacing();
    
    // Checkboxes
    ImGui::Checkbox("Heal Geometry", &m_options.healGeometry);
    ImGui::Checkbox("Sew Faces", &m_options.sewFaces);
    ImGui::Checkbox("Import as Assembly", &m_options.importAsAssembly);
    ImGui::Checkbox("Import Colors", &m_options.importColors);
    ImGui::Checkbox("Import Layers", &m_options.importLayers);
    
    // Advanced
    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::SliderDouble("Sew Tolerance", &m_options.sewTolerance, 0.0001, 0.1, "%.4f");
    }
}

void ImportDialog::renderActionButtons()
{
    float buttonWidth = 120;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float totalWidth = buttonWidth * 2 + spacing;
    
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);
    
    bool canImport = !m_filePath.empty();
    
    ImGui::BeginDisabled(!canImport);
    if (ImGui::Button("Import", ImVec2(buttonWidth, 0))) {
        if (m_importCallback) {
            m_importCallback(m_filePath, m_options);
        }
        close();
    }
    ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
        close();
    }
}

bool ImportDialog::showOpenFileDialog()
{
#ifdef _WIN32
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    
    const char* filter = 
        "CAD Files (*.step;*.stp;*.iges;*.igs;*.dca)\0*.step;*.stp;*.iges;*.igs;*.dca\0"
        "STEP Files (*.step;*.stp)\0*.step;*.stp\0"
        "IGES Files (*.iges;*.igs)\0*.iges;*.igs\0"
        "STL Files (*.stl)\0*.stl\0"
        "OBJ Files (*.obj)\0*.obj\0"
        "DC Design Files (*.dca)\0*.dca\0"
        "All Files (*.*)\0*.*\0";
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = m_defaultDirectory.empty() ? NULL : m_defaultDirectory.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        m_filePath = szFile;
        return true;
    }
#endif
    
    return false;
}

} // namespace dc
