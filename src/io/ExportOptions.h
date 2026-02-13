#pragma once

#include <string>
#include <glm/glm.hpp>

namespace dc {

/**
 * Supported export file formats
 */
enum class ExportFormat {
    STEP_AP203,     // STEP AP203 - geometry only
    STEP_AP214,     // STEP AP214 - with colors/layers
    IGES,           // IGES format
    STL_ASCII,      // STL ASCII format
    STL_BINARY,     // STL binary format
    OBJ,            // Wavefront OBJ
    NATIVE_DCA      // Native .dca format
};

/**
 * Unit systems for export
 */
enum class ExportUnits {
    Millimeters,
    Centimeters,
    Meters,
    Inches,
    Feet
};

/**
 * Coordinate system conventions
 */
enum class CoordinateSystem {
    RightHanded_YUp,    // Standard OpenGL
    RightHanded_ZUp,    // CAD standard (STEP, IGES)
    LeftHanded_YUp      // DirectX convention
};

/**
 * Tessellation quality presets
 */
enum class TessellationQuality {
    Draft,      // Fast, low polygon count
    Standard,   // Balanced
    Fine,       // High quality
    Custom      // User-defined parameters
};

/**
 * Export configuration options
 */
struct ExportOptions {
    // Format settings
    ExportFormat format = ExportFormat::STEP_AP214;
    
    // Unit conversion
    ExportUnits units = ExportUnits::Millimeters;
    double scaleFactor = 1.0;
    
    // Coordinate system
    CoordinateSystem coordSystem = CoordinateSystem::RightHanded_ZUp;
    
    // Tessellation (for mesh export)
    TessellationQuality tessQuality = TessellationQuality::Standard;
    double chordTolerance = 0.1;        // Max deviation from surface
    double angleTolerance = 15.0;       // Max angle between facets (degrees)
    double minEdgeLength = 0.01;        // Minimum triangle edge length
    double maxEdgeLength = 100.0;       // Maximum triangle edge length
    
    // STEP-specific options
    bool includeColors = true;          // Include color information (AP214)
    bool includeLayerInfo = true;       // Include layer assignments
    bool exportAsAssembly = true;       // Export multiple bodies as assembly
    std::string applicationName = "DC-3DDesignApp";
    std::string applicationVersion = "1.0";
    std::string authorName;
    std::string organizationName;
    
    // IGES-specific options
    int igesVersion = 11;               // IGES version (5.3 = 11)
    bool igesIncludeColors = true;
    
    // STL-specific options
    bool stlBinary = true;              // Binary vs ASCII
    bool stlIncludeNormals = true;      // Include vertex normals
    
    // General options
    bool exportHiddenObjects = false;
    bool mergeCoplanarFaces = true;
    bool healGeometry = true;           // Fix small gaps/overlaps
    
    /**
     * Get scale factor for unit conversion
     */
    double getUnitScale() const {
        // Convert from internal units (mm) to export units
        switch (units) {
            case ExportUnits::Millimeters: return 1.0;
            case ExportUnits::Centimeters: return 0.1;
            case ExportUnits::Meters: return 0.001;
            case ExportUnits::Inches: return 1.0 / 25.4;
            case ExportUnits::Feet: return 1.0 / 304.8;
            default: return 1.0;
        }
    }
    
    /**
     * Get transformation matrix for coordinate system conversion
     */
    glm::dmat4 getCoordinateTransform() const {
        glm::dmat4 transform(1.0);
        
        switch (coordSystem) {
            case CoordinateSystem::RightHanded_YUp:
                // No transformation needed (internal format)
                break;
            case CoordinateSystem::RightHanded_ZUp:
                // Rotate -90 degrees around X axis
                transform = glm::dmat4(
                    1.0, 0.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, -1.0, 0.0, 0.0,
                    0.0, 0.0, 0.0, 1.0
                );
                break;
            case CoordinateSystem::LeftHanded_YUp:
                // Mirror X axis
                transform = glm::dmat4(
                    -1.0, 0.0, 0.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0
                );
                break;
        }
        
        return transform;
    }
    
    /**
     * Get tessellation parameters based on quality preset
     */
    void applyQualityPreset(TessellationQuality quality) {
        tessQuality = quality;
        
        switch (quality) {
            case TessellationQuality::Draft:
                chordTolerance = 0.5;
                angleTolerance = 30.0;
                minEdgeLength = 0.1;
                break;
            case TessellationQuality::Standard:
                chordTolerance = 0.1;
                angleTolerance = 15.0;
                minEdgeLength = 0.01;
                break;
            case TessellationQuality::Fine:
                chordTolerance = 0.01;
                angleTolerance = 5.0;
                minEdgeLength = 0.001;
                break;
            case TessellationQuality::Custom:
                // Keep current values
                break;
        }
    }
    
    /**
     * Get file extension for current format
     */
    std::string getFileExtension() const {
        switch (format) {
            case ExportFormat::STEP_AP203:
            case ExportFormat::STEP_AP214:
                return ".step";
            case ExportFormat::IGES:
                return ".igs";
            case ExportFormat::STL_ASCII:
            case ExportFormat::STL_BINARY:
                return ".stl";
            case ExportFormat::OBJ:
                return ".obj";
            case ExportFormat::NATIVE_DCA:
                return ".dca";
            default:
                return "";
        }
    }
    
    /**
     * Get format display name
     */
    std::string getFormatName() const {
        switch (format) {
            case ExportFormat::STEP_AP203: return "STEP AP203";
            case ExportFormat::STEP_AP214: return "STEP AP214";
            case ExportFormat::IGES: return "IGES";
            case ExportFormat::STL_ASCII: return "STL (ASCII)";
            case ExportFormat::STL_BINARY: return "STL (Binary)";
            case ExportFormat::OBJ: return "OBJ";
            case ExportFormat::NATIVE_DCA: return "DC Design (*.dca)";
            default: return "Unknown";
        }
    }
};

/**
 * Import options
 */
struct ImportOptions {
    // Unit assumption for files without unit info
    ExportUnits assumedUnits = ExportUnits::Millimeters;
    
    // Geometry healing
    bool healGeometry = true;
    bool sewFaces = true;
    double sewTolerance = 0.001;
    
    // Import behavior
    bool importAsAssembly = true;       // Keep assembly structure
    bool importColors = true;
    bool importLayers = true;
    
    // Tessellation for visualization
    TessellationQuality tessQuality = TessellationQuality::Standard;
};

} // namespace dc
