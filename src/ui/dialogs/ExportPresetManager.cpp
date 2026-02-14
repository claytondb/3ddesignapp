#include "ExportPresetManager.h"
#include <QSettings>
#include <QCoreApplication>

ExportPresetManager* ExportPresetManager::s_instance = nullptr;

ExportPresetManager* ExportPresetManager::instance()
{
    if (!s_instance) {
        s_instance = new ExportPresetManager(qApp);
    }
    return s_instance;
}

ExportPresetManager::ExportPresetManager(QObject *parent)
    : QObject(parent)
{
    initBuiltInPresets();
    loadUserPresets();
}

ExportPresetManager::~ExportPresetManager()
{
    saveUserPresets();
}

void ExportPresetManager::initBuiltInPresets()
{
    // 3D Printing (STL Binary) - Low tessellation, binary format
    ExportPreset stlPrinting;
    stlPrinting.name = tr("3D Printing (STL Binary)");
    stlPrinting.description = tr("Optimized for 3D printing. Binary STL with standard tessellation.");
    stlPrinting.isBuiltIn = true;
    stlPrinting.format = 0;  // STL Binary
    stlPrinting.stlBinary = true;
    stlPrinting.quality = 0;  // Draft - faster, adequate for printing
    stlPrinting.chordTolerance = 0.5;
    stlPrinting.angleTolerance = 30.0;
    stlPrinting.scaleFactor = 1.0;
    m_builtInPresets[stlPrinting.name] = stlPrinting;
    
    // 3D Printing (High Quality) - High tessellation
    ExportPreset stlHighQuality;
    stlHighQuality.name = tr("3D Printing (High Quality)");
    stlHighQuality.description = tr("High-quality 3D printing. Fine tessellation for smooth surfaces.");
    stlHighQuality.isBuiltIn = true;
    stlHighQuality.format = 0;  // STL Binary
    stlHighQuality.stlBinary = true;
    stlHighQuality.quality = 2;  // Fine
    stlHighQuality.chordTolerance = 0.01;
    stlHighQuality.angleTolerance = 5.0;
    stlHighQuality.scaleFactor = 1.0;
    m_builtInPresets[stlHighQuality.name] = stlHighQuality;
    
    // CAD Exchange (STEP) - Maximum precision
    ExportPreset stepExchange;
    stepExchange.name = tr("CAD Exchange (STEP)");
    stepExchange.description = tr("STEP format for CAD software exchange. Maximum precision.");
    stepExchange.isBuiltIn = true;
    stepExchange.format = 4;  // STEP
    stepExchange.quality = 2;  // Fine
    stepExchange.chordTolerance = 0.001;
    stepExchange.angleTolerance = 1.0;
    stepExchange.scaleFactor = 1.0;
    m_builtInPresets[stepExchange.name] = stepExchange;
    
    // Web/Game (OBJ Low) - Reduced polygons
    ExportPreset objLow;
    objLow.name = tr("Web/Game (OBJ Low)");
    objLow.description = tr("OBJ format optimized for web/game use. Reduced polygon count.");
    objLow.isBuiltIn = true;
    objLow.format = 2;  // OBJ
    objLow.objIncludeNormals = true;
    objLow.objIncludeUVs = true;
    objLow.objIncludeMaterials = false;
    objLow.quality = 0;  // Draft - lower polygon count
    objLow.chordTolerance = 1.0;
    objLow.angleTolerance = 45.0;
    objLow.scaleFactor = 1.0;
    m_builtInPresets[objLow.name] = objLow;
    
    // Set default to 3D Printing (STL Binary) if not already set
    QSettings settings;
    m_defaultPreset = settings.value("Export/DefaultPreset", stlPrinting.name).toString();
}

void ExportPresetManager::loadUserPresets()
{
    QSettings settings;
    int size = settings.beginReadArray("Export/UserPresets");
    
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QVariantMap map = settings.value("preset").toMap();
        ExportPreset preset = ExportPreset::fromVariantMap(map);
        preset.isBuiltIn = false;
        if (!preset.name.isEmpty()) {
            m_userPresets[preset.name] = preset;
        }
    }
    
    settings.endArray();
}

void ExportPresetManager::saveUserPresets()
{
    QSettings settings;
    settings.beginWriteArray("Export/UserPresets");
    
    int i = 0;
    for (auto it = m_userPresets.begin(); it != m_userPresets.end(); ++it, ++i) {
        settings.setArrayIndex(i);
        settings.setValue("preset", it.value().toVariantMap());
    }
    
    settings.endArray();
    settings.setValue("Export/DefaultPreset", m_defaultPreset);
}

QStringList ExportPresetManager::presetNames() const
{
    QStringList names;
    names << m_builtInPresets.keys();
    names << m_userPresets.keys();
    return names;
}

QStringList ExportPresetManager::userPresetNames() const
{
    return m_userPresets.keys();
}

QStringList ExportPresetManager::builtInPresetNames() const
{
    return m_builtInPresets.keys();
}

bool ExportPresetManager::hasPreset(const QString& name) const
{
    return m_builtInPresets.contains(name) || m_userPresets.contains(name);
}

bool ExportPresetManager::isBuiltIn(const QString& name) const
{
    return m_builtInPresets.contains(name);
}

ExportPresetManager::ExportPreset ExportPresetManager::preset(const QString& name) const
{
    if (m_builtInPresets.contains(name)) {
        return m_builtInPresets[name];
    }
    if (m_userPresets.contains(name)) {
        return m_userPresets[name];
    }
    // Return first built-in preset as fallback
    if (!m_builtInPresets.isEmpty()) {
        return m_builtInPresets.first();
    }
    return ExportPreset();
}

bool ExportPresetManager::savePreset(const ExportPreset& preset)
{
    if (preset.name.isEmpty()) {
        return false;
    }
    
    // Cannot overwrite built-in presets
    if (m_builtInPresets.contains(preset.name)) {
        return false;
    }
    
    ExportPreset p = preset;
    p.isBuiltIn = false;
    m_userPresets[preset.name] = p;
    saveUserPresets();
    emit presetsChanged();
    return true;
}

bool ExportPresetManager::deletePreset(const QString& name)
{
    // Cannot delete built-in presets
    if (m_builtInPresets.contains(name)) {
        return false;
    }
    
    if (m_userPresets.remove(name) > 0) {
        // If deleted preset was default, reset to first built-in
        if (m_defaultPreset == name && !m_builtInPresets.isEmpty()) {
            setDefaultPreset(m_builtInPresets.firstKey());
        }
        saveUserPresets();
        emit presetsChanged();
        return true;
    }
    return false;
}

bool ExportPresetManager::renamePreset(const QString& oldName, const QString& newName)
{
    if (oldName.isEmpty() || newName.isEmpty() || oldName == newName) {
        return false;
    }
    
    // Cannot rename built-in presets
    if (m_builtInPresets.contains(oldName)) {
        return false;
    }
    
    // Cannot rename to existing name
    if (hasPreset(newName)) {
        return false;
    }
    
    if (m_userPresets.contains(oldName)) {
        ExportPreset preset = m_userPresets.take(oldName);
        preset.name = newName;
        m_userPresets[newName] = preset;
        
        // Update default if renamed
        if (m_defaultPreset == oldName) {
            m_defaultPreset = newName;
        }
        
        saveUserPresets();
        emit presetsChanged();
        return true;
    }
    return false;
}

QString ExportPresetManager::defaultPreset() const
{
    return m_defaultPreset;
}

void ExportPresetManager::setDefaultPreset(const QString& name)
{
    if (hasPreset(name) && m_defaultPreset != name) {
        m_defaultPreset = name;
        QSettings settings;
        settings.setValue("Export/DefaultPreset", m_defaultPreset);
        emit defaultPresetChanged(m_defaultPreset);
    }
}

ExportPresetManager::ExportPreset ExportPresetManager::quickExportPreset() const
{
    return preset(m_defaultPreset);
}

// ExportPreset serialization
QVariantMap ExportPresetManager::ExportPreset::toVariantMap() const
{
    QVariantMap map;
    map["name"] = name;
    map["description"] = description;
    map["format"] = format;
    map["stlBinary"] = stlBinary;
    map["objIncludeNormals"] = objIncludeNormals;
    map["objIncludeUVs"] = objIncludeUVs;
    map["objIncludeMaterials"] = objIncludeMaterials;
    map["plyBinary"] = plyBinary;
    map["plyIncludeColors"] = plyIncludeColors;
    map["quality"] = quality;
    map["chordTolerance"] = chordTolerance;
    map["angleTolerance"] = angleTolerance;
    map["exportSelected"] = exportSelected;
    map["scaleFactor"] = scaleFactor;
    return map;
}

ExportPresetManager::ExportPreset ExportPresetManager::ExportPreset::fromVariantMap(const QVariantMap& map)
{
    ExportPreset preset;
    preset.name = map.value("name").toString();
    preset.description = map.value("description").toString();
    preset.format = map.value("format", 0).toInt();
    preset.stlBinary = map.value("stlBinary", true).toBool();
    preset.objIncludeNormals = map.value("objIncludeNormals", true).toBool();
    preset.objIncludeUVs = map.value("objIncludeUVs", false).toBool();
    preset.objIncludeMaterials = map.value("objIncludeMaterials", false).toBool();
    preset.plyBinary = map.value("plyBinary", true).toBool();
    preset.plyIncludeColors = map.value("plyIncludeColors", true).toBool();
    preset.quality = map.value("quality", 1).toInt();
    preset.chordTolerance = map.value("chordTolerance", 0.1).toDouble();
    preset.angleTolerance = map.value("angleTolerance", 15.0).toDouble();
    preset.exportSelected = map.value("exportSelected", false).toBool();
    preset.scaleFactor = map.value("scaleFactor", 1.0).toDouble();
    return preset;
}
