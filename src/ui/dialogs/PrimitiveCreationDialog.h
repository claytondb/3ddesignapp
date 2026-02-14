/**
 * @file PrimitiveCreationDialog.h
 * @brief Dialog for creating primitives with size presets and custom dimensions
 * 
 * Provides a Tinkercad-like experience for primitive creation:
 * - Size presets (Small, Medium, Large)
 * - Custom dimensions input
 * - Preview of settings
 */

#ifndef DC_PRIMITIVECREATIONDIALOG_H
#define DC_PRIMITIVECREATIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <glm/glm.hpp>

/**
 * @brief Dialog for configuring primitive creation parameters
 */
class PrimitiveCreationDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Primitive types supported
     */
    enum class PrimitiveType {
        Cube,
        Sphere,
        Cylinder,
        Cone,
        Plane,
        Torus
    };
    
    /**
     * @brief Size presets
     */
    enum class SizePreset {
        Small,   // 1 unit
        Medium,  // 2 units (default)
        Large,   // 5 units
        Custom   // User-defined
    };
    
    /**
     * @brief Configuration result
     */
    struct PrimitiveConfig {
        PrimitiveType type;
        
        // Dimensions (interpretation depends on type)
        float width = 2.0f;     // X dimension / radius
        float height = 2.0f;    // Y dimension / height  
        float depth = 2.0f;     // Z dimension / minor radius
        
        // Resolution (for curved surfaces)
        int segments = 32;
        
        // Position (if positionAtCursor is false)
        glm::vec3 position = glm::vec3(0.0f);
        
        // Options
        bool positionAtCursor = false;
        bool positionAtViewCenter = true;
        bool selectAfterCreation = true;
    };

    explicit PrimitiveCreationDialog(PrimitiveType type, QWidget* parent = nullptr);
    ~PrimitiveCreationDialog() override = default;
    
    /**
     * @brief Get the configured primitive settings
     */
    PrimitiveConfig config() const;
    
    /**
     * @brief Set the view center position (for positioning primitives)
     */
    void setViewCenter(const glm::vec3& center);
    
    /**
     * @brief Static convenience method to create and show dialog
     * @return true if user accepted, config is populated
     */
    static bool getConfig(PrimitiveType type, PrimitiveConfig& outConfig, 
                          QWidget* parent = nullptr);

private slots:
    void onPresetChanged(int index);
    void onDimensionsChanged();
    void updatePreview();

private:
    void setupUI();
    void setupConnections();
    void updateDimensionsForType();
    void applyPreset(SizePreset preset);
    QString typeToString(PrimitiveType type) const;
    
    PrimitiveType m_type;
    glm::vec3 m_viewCenter;
    
    // UI elements
    QComboBox* m_presetCombo;
    QGroupBox* m_dimensionsGroup;
    QDoubleSpinBox* m_widthSpin;
    QDoubleSpinBox* m_heightSpin;
    QDoubleSpinBox* m_depthSpin;
    QLabel* m_widthLabel;
    QLabel* m_heightLabel;
    QLabel* m_depthLabel;
    QSpinBox* m_segmentsSpin;
    QLabel* m_segmentsLabel;
    
    // Position options
    QComboBox* m_positionCombo;
    
    // Preview
    QLabel* m_previewLabel;
    
    // Options
    QCheckBox* m_selectAfterCheck;
    
    // Buttons
    QPushButton* m_createButton;
    QPushButton* m_cancelButton;
};

#endif // DC_PRIMITIVECREATIONDIALOG_H
