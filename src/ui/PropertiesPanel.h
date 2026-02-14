#ifndef DC_PROPERTIESPANEL_H
#define DC_PROPERTIESPANEL_H

#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QGroupBox>

/**
 * @brief Properties panel showing context-sensitive object properties
 * 
 * Uses QStackedWidget to show different pages based on selection:
 * - No selection: Scene statistics
 * - Mesh selected: Mesh properties
 * - Primitive selected: Primitive parameters
 * - Surface selected: Surface properties
 * - Body selected: Body properties
 */
class PropertiesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget *parent = nullptr);
    ~PropertiesPanel() override = default;

    // Page indices
    enum Page {
        PageNoSelection = 0,
        PageMesh,
        PagePrimitive,
        PageSketch,
        PageSurface,
        PageBody
    };

    // Set current page
    void setPage(Page page);
    Page currentPage() const;

    // Generic property setting (from IntegrationController)
    void setProperties(const QVariantMap& props);
    void clearProperties();

    // Update scene statistics (no selection page)
    void setSceneStats(int meshCount, int triangleCount, int surfaceCount, int bodyCount);
    void setUnits(const QString& units);

    // Update mesh properties
    void setMeshName(const QString& name);
    void setMeshTriangles(int count);
    void setMeshVertices(int count);
    void setMeshEdges(int count);
    void setMeshBounds(double x, double y, double z);
    void setMeshHasHoles(bool hasHoles, int holeCount);
    void setMeshMemoryUsage(size_t bytes);
    void setMeshColor(const QColor& color);
    void setMeshOpacity(int percent);
    void setMeshShowEdges(bool show);
    void setMeshPosition(double x, double y, double z);
    void setMeshRotation(double x, double y, double z);

    // Update deviation info (when enabled)
    void setDeviationStats(double min, double max, double avg, double stdDev);
    void showDeviation(bool show);

signals:
    // Property changes
    void meshColorChanged(const QColor& color);
    void meshOpacityChanged(int percent);
    void meshShowEdgesChanged(bool show);
    void meshPositionChanged(double x, double y, double z);
    void meshRotationChanged(double x, double y, double z);
    void unitsChanged(const QString& units);

private:
    void setupUI();
    QWidget* createNoSelectionPage();
    QWidget* createMeshPage();
    QWidget* createPrimitivePage();
    QWidget* createSketchPage();
    QWidget* createSurfacePage();
    QWidget* createBodyPage();

    QGroupBox* createCollapsibleGroup(const QString& title);
    QWidget* createSpinBoxRow(const QString& label, QDoubleSpinBox* spinBox, 
                               const QString& suffix = QString());

    // Layout
    QVBoxLayout* m_layout;
    QStackedWidget* m_stackedWidget;

    // No selection page widgets
    QLabel* m_meshCountLabel;
    QLabel* m_triangleCountLabel;
    QLabel* m_surfaceCountLabel;
    QLabel* m_bodyCountLabel;
    QComboBox* m_unitsCombo;

    // Mesh page widgets
    QLabel* m_meshNameLabel;
    QLabel* m_meshTrianglesLabel;
    QLabel* m_meshVerticesLabel;
    QLabel* m_meshEdgesLabel;
    QLabel* m_meshBoundsLabel;
    QLabel* m_meshHolesLabel;
    QLabel* m_meshMemoryLabel;
    QSlider* m_opacitySlider;
    QLabel* m_opacityLabel;
    QCheckBox* m_showEdgesCheck;
    QDoubleSpinBox* m_posXSpin;
    QDoubleSpinBox* m_posYSpin;
    QDoubleSpinBox* m_posZSpin;
    QDoubleSpinBox* m_rotXSpin;
    QDoubleSpinBox* m_rotYSpin;
    QDoubleSpinBox* m_rotZSpin;

    // Deviation widgets
    QGroupBox* m_deviationGroup;
    QLabel* m_deviationMinLabel;
    QLabel* m_deviationMaxLabel;
    QLabel* m_deviationAvgLabel;
    QLabel* m_deviationStdLabel;
};

#endif // DC_PROPERTIESPANEL_H
