#pragma once

#include "ExportOptions.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <glm/glm.hpp>

namespace dc {

// Forward declarations
class Model;
class Body;
class Face;
class Edge;
class NURBSSurface;
class NURBSCurve;

/**
 * Parsed IGES directory entry
 */
struct IGESDirectoryEntry {
    int entityType;
    int parameterData;
    int structure;
    int lineFontPattern;
    int level;
    int view;
    int transformationMatrix;
    int labelDisplayAssoc;
    int statusNumber;
    int lineWeight;
    int colorNumber;
    int parameterLineCount;
    int formNumber;
    std::string entityLabel;
    std::string entitySubscript;
    int sequenceNumber;
};

/**
 * Parsed IGES parameter entry
 */
struct IGESParameterEntry {
    int entityType;
    std::vector<std::string> parameters;
    int directoryEntry;
};

/**
 * IGES file importer
 * Imports IGES format files
 */
class IGESImporter {
public:
    IGESImporter();
    ~IGESImporter();
    
    /**
     * Import an IGES file
     * @param filename Input file path
     * @param options Import options
     * @return Imported model, or nullptr on failure
     */
    std::shared_ptr<Model> importFile(const std::string& filename, const ImportOptions& options);
    
    /**
     * Get last error message
     */
    const std::string& getErrorMessage() const { return m_errorMessage; }
    
    /**
     * Get import statistics
     */
    struct ImportStats {
        int totalEntities = 0;
        int curvesImported = 0;
        int surfacesImported = 0;
        int pointsImported = 0;
        std::vector<std::string> warnings;
    };
    
    const ImportStats& getStats() const { return m_stats; }
    
private:
    // File sections
    std::string m_startSection;
    std::string m_globalSection;
    std::vector<IGESDirectoryEntry> m_directoryEntries;
    std::map<int, IGESParameterEntry> m_parameterEntries;  // Keyed by directory entry number
    
    // Parsed data
    std::map<int, glm::dvec3> m_points;
    std::map<int, std::shared_ptr<NURBSCurve>> m_curves;
    std::map<int, std::shared_ptr<NURBSSurface>> m_surfaces;
    std::map<int, glm::dmat4> m_transformations;
    std::map<int, glm::vec3> m_colors;
    
    // Global parameters
    char m_paramDelimiter = ',';
    char m_recordDelimiter = ';';
    std::string m_productId;
    std::string m_fileName;
    double m_modelScale = 1.0;
    int m_unitsFlag = 1;
    double m_maxCoordValue = 0.0;
    
    std::string m_errorMessage;
    ImportStats m_stats;
    ImportOptions m_options;
    
    // Parsing
    bool parseFile(const std::string& filename);
    bool parseStartSection(const std::vector<std::string>& lines);
    bool parseGlobalSection(const std::vector<std::string>& lines);
    bool parseDirectorySection(const std::vector<std::string>& lines);
    bool parseParameterSection(const std::vector<std::string>& lines);
    
    std::vector<std::string> parseParameterData(const std::string& data);
    
    // Entity processing
    bool processEntities();
    
    // Geometry extraction
    glm::dvec3 getPoint(int directoryEntry);
    std::shared_ptr<NURBSCurve> getLine(int directoryEntry);
    std::shared_ptr<NURBSCurve> getCircularArc(int directoryEntry);
    std::shared_ptr<NURBSCurve> getConicArc(int directoryEntry);
    std::shared_ptr<NURBSCurve> getCompositeCurve(int directoryEntry);
    std::shared_ptr<NURBSCurve> getRationalBSplineCurve(int directoryEntry);
    
    std::shared_ptr<NURBSSurface> getPlane(int directoryEntry);
    std::shared_ptr<NURBSSurface> getRuledSurface(int directoryEntry);
    std::shared_ptr<NURBSSurface> getSurfaceOfRevolution(int directoryEntry);
    std::shared_ptr<NURBSSurface> getTabulatedCylinder(int directoryEntry);
    std::shared_ptr<NURBSSurface> getRationalBSplineSurface(int directoryEntry);
    std::shared_ptr<NURBSSurface> getTrimmedSurface(int directoryEntry);
    
    glm::dmat4 getTransformationMatrix(int directoryEntry);
    glm::vec3 getColorDefinition(int directoryEntry);
    
    // Utility functions
    double parseReal(const std::string& str);
    int parseInt(const std::string& str);
    std::string parseHollerith(const std::string& str);
    double getUnitScale() const;
    
    void addWarning(const std::string& msg);
};

} // namespace dc
