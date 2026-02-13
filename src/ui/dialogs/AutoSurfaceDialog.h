#pragma once

#include "Dialog.h"
#include "../../geometry/freeform/AutoSurface.h"
#include <memory>
#include <thread>
#include <atomic>

namespace dc {

class TriangleMesh;
class QuadMesh;
class SceneObject;
class Viewport;

/**
 * @brief Dialog for automatic quad mesh and surface generation
 */
class AutoSurfaceDialog : public Dialog {
public:
    AutoSurfaceDialog();
    ~AutoSurfaceDialog() override;
    
    // Dialog interface
    void show() override;
    void hide() override;
    bool isVisible() const override { return m_visible; }
    
    void render() override;
    void update(float deltaTime) override;
    
    // Input mesh
    void setInputMesh(std::shared_ptr<TriangleMesh> mesh);
    void setInputObject(std::shared_ptr<SceneObject> object);
    
    // Result access
    std::unique_ptr<QuadMesh> getQuadMeshResult();
    std::vector<std::unique_ptr<NurbsSurface>> getSurfaceResults();
    
    // Callbacks
    using ResultCallback = std::function<void(bool success, const std::string& message)>;
    void setResultCallback(ResultCallback callback) { m_resultCallback = callback; }
    
private:
    bool m_visible = false;
    
    // Input
    std::shared_ptr<TriangleMesh> m_inputMesh;
    std::shared_ptr<SceneObject> m_inputObject;
    
    // Parameters
    AutoSurfaceParams m_params;
    
    // Processing
    std::unique_ptr<AutoSurface> m_autoSurface;
    std::thread m_processingThread;
    std::atomic<bool> m_isProcessing{false};
    std::atomic<float> m_progress{0.0f};
    std::string m_progressStage;
    std::mutex m_progressMutex;
    
    // Results
    std::unique_ptr<QuadMesh> m_resultQuadMesh;
    std::vector<std::unique_ptr<NurbsSurface>> m_resultSurfaces;
    AutoSurfaceMetrics m_metrics;
    
    // Preview
    std::shared_ptr<Viewport> m_previewViewport;
    bool m_showPreview = true;
    int m_previewSubdivLevel = 1;
    
    // Callback
    ResultCallback m_resultCallback;
    
    // UI sections
    void renderParameterSection();
    void renderFeatureSection();
    void renderContinuitySection();
    void renderOutputSection();
    void renderPreviewSection();
    void renderProgressSection();
    void renderResultsSection();
    void renderButtons();
    
    // Actions
    void startProcessing();
    void cancelProcessing();
    void applyResults();
    void previewResults();
    
    // Processing thread
    void processAsync();
    void onProgressUpdate(float progress, const std::string& stage);
};

/**
 * @brief Simplified auto-surface wizard for beginners
 */
class AutoSurfaceWizard : public Dialog {
public:
    AutoSurfaceWizard();
    ~AutoSurfaceWizard() override;
    
    void show() override;
    void hide() override;
    bool isVisible() const override { return m_visible; }
    
    void render() override;
    void update(float deltaTime) override;
    
    void setInputMesh(std::shared_ptr<TriangleMesh> mesh);
    
private:
    bool m_visible = false;
    std::shared_ptr<TriangleMesh> m_inputMesh;
    
    // Wizard state
    int m_currentStep = 0;
    static const int STEP_COUNT = 4;
    
    // Simplified parameters
    enum class QualityPreset {
        Draft,      // Fast, lower quality
        Standard,   // Balanced
        High,       // Higher quality, slower
        Custom      // Show advanced options
    };
    QualityPreset m_qualityPreset = QualityPreset::Standard;
    
    float m_detailLevel = 0.5f;         // 0-1, affects patch count
    float m_featureSharpness = 0.5f;    // 0-1, affects crease detection
    bool m_generateNurbs = true;
    
    // Derived parameters
    AutoSurfaceParams m_params;
    
    // Processing
    std::unique_ptr<AutoSurface> m_autoSurface;
    std::thread m_processingThread;
    std::atomic<bool> m_isProcessing{false};
    std::atomic<float> m_progress{0.0f};
    
    // Results
    std::unique_ptr<QuadMesh> m_resultQuadMesh;
    
    // UI
    void renderStepIndicator();
    void renderStep0_Introduction();
    void renderStep1_Quality();
    void renderStep2_Preview();
    void renderStep3_Complete();
    void renderNavigationButtons();
    
    void nextStep();
    void prevStep();
    void generatePreview();
    void finalize();
    
    void updateParamsFromPreset();
};

} // namespace dc
