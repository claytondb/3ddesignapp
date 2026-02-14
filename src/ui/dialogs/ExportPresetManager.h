#ifndef DC_EXPORTPRESETMANAGER_H
#define DC_EXPORTPRESETMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

/**
 * @brief Manages export presets stored in QSettings
 * 
 * Provides functionality to:
 * - Save/load user-defined presets
 * - Manage built-in presets
 * - Set default preset for Quick Export
 */
class ExportPresetManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Preset data structure
     */
    struct ExportPreset {
        QString name;
        QString description;
        bool isBuiltIn = false;
        
        // Format settings
        int format = 0;  // 0=STL Binary, 1=STL ASCII, 2=OBJ, 3=PLY, 4=STEP, 5=IGES
        
        // STL options
        bool stlBinary = true;
        
        // OBJ options
        bool objIncludeNormals = true;
        bool objIncludeUVs = false;
        bool objIncludeMaterials = false;
        
        // PLY options
        bool plyBinary = true;
        bool plyIncludeColors = true;
        
        // Quality settings
        int quality = 1;  // 0=Draft, 1=Standard, 2=Fine, 3=Custom
        double chordTolerance = 0.1;
        double angleTolerance = 15.0;
        
        // General
        bool exportSelected = false;
        double scaleFactor = 1.0;
        
        // Serialize to/from QVariantMap for QSettings
        QVariantMap toVariantMap() const;
        static ExportPreset fromVariantMap(const QVariantMap& map);
    };
    
    /**
     * @brief Get singleton instance
     */
    static ExportPresetManager* instance();
    
    /**
     * @brief Get all preset names (built-in + user)
     */
    QStringList presetNames() const;
    
    /**
     * @brief Get user-defined preset names only
     */
    QStringList userPresetNames() const;
    
    /**
     * @brief Get built-in preset names only
     */
    QStringList builtInPresetNames() const;
    
    /**
     * @brief Check if a preset exists
     */
    bool hasPreset(const QString& name) const;
    
    /**
     * @brief Check if a preset is built-in
     */
    bool isBuiltIn(const QString& name) const;
    
    /**
     * @brief Get a preset by name
     */
    ExportPreset preset(const QString& name) const;
    
    /**
     * @brief Save a user preset
     * @return true if saved successfully
     */
    bool savePreset(const ExportPreset& preset);
    
    /**
     * @brief Delete a user preset
     * @return true if deleted (returns false for built-in presets)
     */
    bool deletePreset(const QString& name);
    
    /**
     * @brief Rename a user preset
     * @return true if renamed successfully
     */
    bool renamePreset(const QString& oldName, const QString& newName);
    
    /**
     * @brief Get the default preset name
     */
    QString defaultPreset() const;
    
    /**
     * @brief Set the default preset for Quick Export
     */
    void setDefaultPreset(const QString& name);
    
    /**
     * @brief Get preset for Quick Export
     */
    ExportPreset quickExportPreset() const;

signals:
    void presetsChanged();
    void defaultPresetChanged(const QString& name);

private:
    explicit ExportPresetManager(QObject *parent = nullptr);
    ~ExportPresetManager() override;
    
    void initBuiltInPresets();
    void loadUserPresets();
    void saveUserPresets();
    
    QMap<QString, ExportPreset> m_builtInPresets;
    QMap<QString, ExportPreset> m_userPresets;
    QString m_defaultPreset;
    
    static ExportPresetManager* s_instance;
};

#endif // DC_EXPORTPRESETMANAGER_H
