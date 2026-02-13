#pragma once

#include "Dialog.h"
#include "../../geometry/freeform/SurfaceFit.h"
#include <memory>
#include <thread>
#include <atomic>

namespace dc {

class TriangleMesh;
class NurbsSurface;
class NurbsCurve;
class SceneObject;
class Selection;

/**
 * @brief Dialog for fitting NURBS surfaces to mesh regions
 */
class FitSurfaceDialog : public Dialog {
public:
    FitSurfaceDialog();
    ~FitSurfaceDialog() override;
    
    // Dialog interface
    void show() override;
    void hide() override;
    bool isVisible() const override { return m_visible; }
    
    void render() override;
    void update(float deltaTime) override;
    
    // Input
    void setInputMesh(std::shared_ptr<TriangleMesh> mesh);
    void setSelectedFaces(const std::vector<int>& faceIndices);
    void setBoundaryCurves(const std::vector<std::shared_ptr<NurbsCurve>>& curves);
    
    // Result
    std::unique_ptr<NurbsSurface> getResult();
    
    // Callback
    using ResultCallback = std::function<void(bool success)>;
    void setResultCallback(ResultCallback callback) { m_resultCallback = callback; }
    
private:
    bool m_visible = false;
    
    // Input
    std::shared_ptr<TriangleMesh> m_inputMesh;
    std::vector<int> m_selectedFaces;
    std::vector<std::shared_ptr<NurbsCurve>> m_boundaryCurves;
    
    // Parameters
    SurfaceFitParams m_params;
    
    // Boundary condition UI state
    struct BoundaryUI {
        int conditionType = 0;  // Index into combo
        bool useCurve = false;
        int curveIndex = -1;
    };
    BoundaryUI m_boundaryUI[4];  // uMin, uMax, vMin, vMax
    
    // Processing
    std::unique_ptr<SurfaceFitter> m_fitter;
    std::thread m_processingThread;
    std::atomic<bool> m_isProcessing{false};
    std::atomic<float> m_progress{0.0f};
    std::atomic<float> m_currentDeviation{0.0f};
    
    // Result
    std::unique_ptr<NurbsSurface> m_resultSurface;
    SurfaceFitResult m_fitResult;
    
    // Preview
    bool m_showPreview = true;
    bool m_showDeviation = false;
    
    // Callback
    ResultCallback m_resultCallback;
    
    // UI sections
    void renderRegionSection();
    void renderSurfaceSection();
    void renderToleranceSection();
    void renderBoundarySection();
    void renderAdvancedSection();
    void renderResultsSection();
    void renderButtons();
    
    // Actions
    void startFitting();
    void cancelFitting();
    void applyResult();
    
    // Processing
    void processAsync();
    void onProgressUpdate(float progress, float deviation);
    
    // Helpers
    BoundaryCondition boundaryConditionFromUI(int index) const;
    const char* boundaryConditionName(BoundaryCondition bc) const;
};

/**
 * @brief Dialog for fitting surfaces to curve networks
 */
class CurveNetworkFitDialog : public Dialog {
public:
    CurveNetworkFitDialog();
    ~CurveNetworkFitDialog() override;
    
    void show() override;
    void hide() override;
    bool isVisible() const override { return m_visible; }
    
    void render() override;
    void update(float deltaTime) override;
    
    void setCurves(const std::vector<std::shared_ptr<NurbsCurve>>& curves);
    std::unique_ptr<NurbsSurface> getResult();
    
private:
    bool m_visible = false;
    
    std::vector<std::shared_ptr<NurbsCurve>> m_curves;
    SurfaceFitParams m_params;
    
    std::unique_ptr<SurfaceFitter> m_fitter;
    std::thread m_processingThread;
    std::atomic<bool> m_isProcessing{false};
    
    std::unique_ptr<NurbsSurface> m_resultSurface;
    
    // Curve classification
    std::vector<int> m_uCurveIndices;
    std::vector<int> m_vCurveIndices;
    bool m_autoClassify = true;
    
    void renderCurveList();
    void renderClassificationSection();
    void renderParameterSection();
    void renderButtons();
    
    void autoClassifyCurves();
    void startFitting();
};

/**
 * @brief Quick surface fit tool - simplified interface
 */
class QuickFitSurfaceDialog : public Dialog {
public:
    QuickFitSurfaceDialog();
    ~QuickFitSurfaceDialog() override;
    
    void show() override;
    void hide() override;
    bool isVisible() const override { return m_visible; }
    
    void render() override;
    void update(float deltaTime) override;
    
    void setSelection(std::shared_ptr<Selection> selection);
    
private:
    bool m_visible = false;
    std::shared_ptr<Selection> m_selection;
    
    // Simplified parameters
    int m_quality = 1;  // 0=Low, 1=Medium, 2=High
    int m_degree = 3;
    bool m_matchBoundaries = true;
    
    std::unique_ptr<SurfaceFitter> m_fitter;
    std::atomic<bool> m_isProcessing{false};
    
    std::unique_ptr<NurbsSurface> m_result;
    
    void renderQualitySelector();
    void renderPreview();
    void startFitting();
};

} // namespace dc
