#include "AutoSurfaceDialog.h"
#include "../../geometry/mesh/TriangleMesh.h"
#include "../../geometry/freeform/QuadMesh.h"
#include "../../geometry/nurbs/NurbsSurface.h"
#include "../../scene/SceneObject.h"
#include "../imgui/imgui.h"

namespace dc {

AutoSurfaceDialog::AutoSurfaceDialog() {
    m_autoSurface = std::make_unique<AutoSurface>();
}

AutoSurfaceDialog::~AutoSurfaceDialog() {
    cancelProcessing();
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }
}

void AutoSurfaceDialog::show() {
    m_visible = true;
    
    // Suggest parameters based on input mesh
    if (m_inputMesh) {
        m_params = AutoSurfaceUtils::suggestParameters(*m_inputMesh);
    }
}

void AutoSurfaceDialog::hide() {
    m_visible = false;
    cancelProcessing();
}

void AutoSurfaceDialog::setInputMesh(std::shared_ptr<TriangleMesh> mesh) {
    m_inputMesh = mesh;
    m_resultQuadMesh.reset();
    m_resultSurfaces.clear();
    
    if (mesh) {
        m_params = AutoSurfaceUtils::suggestParameters(*mesh);
    }
}

void AutoSurfaceDialog::setInputObject(std::shared_ptr<SceneObject> object) {
    m_inputObject = object;
    // Extract mesh from object
    // m_inputMesh = object->getTriangleMesh();
}

void AutoSurfaceDialog::render() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Auto Surface", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        if (m_isProcessing) {
            renderProgressSection();
        } else {
            // Input info
            if (m_inputMesh) {
                ImGui::Text("Input: %d vertices, %d triangles",
                           m_inputMesh->vertexCount(), m_inputMesh->triangleCount());
            } else {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No input mesh selected");
            }
            ImGui::Separator();
            
            // Tabs for different sections
            if (ImGui::BeginTabBar("AutoSurfaceTabs")) {
                if (ImGui::BeginTabItem("Parameters")) {
                    renderParameterSection();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Features")) {
                    renderFeatureSection();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Continuity")) {
                    renderContinuitySection();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Output")) {
                    renderOutputSection();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            
            ImGui::Separator();
            
            if (m_resultQuadMesh) {
                renderResultsSection();
                ImGui::Separator();
            }
            
            renderButtons();
        }
    }
    ImGui::End();
    
    // Preview window
    if (m_showPreview && m_resultQuadMesh) {
        renderPreviewSection();
    }
}

void AutoSurfaceDialog::update(float deltaTime) {
    // Check if processing completed
    if (m_processingThread.joinable() && !m_isProcessing) {
        m_processingThread.join();
        
        // Get results
        m_resultQuadMesh = m_autoSurface->getQuadMesh();
        m_resultSurfaces = m_autoSurface->getSurfaces();
        m_metrics = m_autoSurface->getMetrics();
        
        if (m_resultCallback) {
            m_resultCallback(m_resultQuadMesh != nullptr, "Processing complete");
        }
    }
}

void AutoSurfaceDialog::renderParameterSection() {
    ImGui::Text("Target Mesh Quality");
    ImGui::Separator();
    
    ImGui::SliderInt("Target Patch Count", &m_params.targetPatchCount, 10, 500);
    ImGui::SetItemTooltip("Approximate number of quad faces in output");
    
    float devMm = m_params.deviationTolerance * 1000.0f;
    if (ImGui::SliderFloat("Deviation Tolerance (mm)", &devMm, 0.01f, 10.0f)) {
        m_params.deviationTolerance = devMm / 1000.0f;
    }
    ImGui::SetItemTooltip("Maximum allowed distance from original surface");
    
    ImGui::Spacing();
    ImGui::Text("Optimization");
    ImGui::Separator();
    
    ImGui::SliderInt("Max Iterations", &m_params.maxIterations, 10, 500);
    
    float convThreshold = m_params.convergenceThreshold * 10000.0f;
    if (ImGui::SliderFloat("Convergence (x10000)", &convThreshold, 0.1f, 100.0f)) {
        m_params.convergenceThreshold = convThreshold / 10000.0f;
    }
    
    ImGui::Checkbox("Optimize Flow", &m_params.optimizeFlow);
    ImGui::SetItemTooltip("Align quad edges with principal curvature directions");
}

void AutoSurfaceDialog::renderFeatureSection() {
    ImGui::Text("Feature Detection");
    ImGui::Separator();
    
    ImGui::Checkbox("Detect Creases", &m_params.detectCreases);
    if (m_params.detectCreases) {
        ImGui::SliderFloat("Crease Angle (deg)", &m_params.featureAngleThreshold, 10.0f, 90.0f);
        ImGui::SetItemTooltip("Edges with dihedral angle above this are marked as creases");
    }
    
    ImGui::Checkbox("Detect Corners", &m_params.detectCorners);
    
    ImGui::SliderFloat("Feature Preservation", &m_params.featurePreservation, 0.0f, 1.0f);
    ImGui::SetItemTooltip("How strictly to preserve detected features (0=ignore, 1=strict)");
    
    ImGui::Spacing();
    
    // Show detected features preview
    if (m_inputMesh && ImGui::Button("Analyze Features")) {
        m_autoSurface->setInput(*m_inputMesh);
        m_autoSurface->detectFeatures(m_params);
    }
    
    auto& featureEdges = m_autoSurface->getFeatureEdges();
    auto& featurePoints = m_autoSurface->getFeaturePoints();
    
    if (!featureEdges.empty() || !featurePoints.empty()) {
        ImGui::Text("Detected: %zu edges, %zu corners",
                   featureEdges.size(), featurePoints.size());
    }
}

void AutoSurfaceDialog::renderContinuitySection() {
    ImGui::Text("Surface Continuity");
    ImGui::Separator();
    
    const char* continuityItems[] = { "G0 (Position)", "G1 (Tangent)", "G2 (Curvature)" };
    ImGui::Combo("Target Continuity", &m_params.targetContinuity, continuityItems, 3);
    ImGui::SetItemTooltip("Smoothness between surface patches");
    
    if (m_params.targetContinuity >= 2) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), 
                          "Note: G2 continuity requires more patches and processing time");
    }
}

void AutoSurfaceDialog::renderOutputSection() {
    ImGui::Text("Output Options");
    ImGui::Separator();
    
    ImGui::Checkbox("Generate NURBS Surfaces", &m_params.generateNurbs);
    
    if (m_params.generateNurbs) {
        ImGui::SliderInt("Surface Degree", &m_params.nurbsDegree, 2, 5);
        ImGui::SetItemTooltip("Polynomial degree of output NURBS surfaces");
    }
}

void AutoSurfaceDialog::renderPreviewSection() {
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Auto Surface Preview", &m_showPreview)) {
        // Preview controls
        ImGui::SliderInt("Subdivision Preview", &m_previewSubdivLevel, 0, 3);
        
        // Render preview viewport
        if (m_previewViewport) {
            m_previewViewport->render();
        }
        
        // Quality metrics
        if (m_resultQuadMesh) {
            auto quality = m_resultQuadMesh->computeQuality();
            
            ImGui::Separator();
            ImGui::Text("Quality Metrics:");
            ImGui::Text("  Quad %%: %.1f", quality.quadPercentage);
            ImGui::Text("  Min Angle: %.1f°", quality.minAngle);
            ImGui::Text("  Max Angle: %.1f°", quality.maxAngle);
            ImGui::Text("  Irregular Vertices: %d", quality.irregularVertices);
        }
    }
    ImGui::End();
}

void AutoSurfaceDialog::renderProgressSection() {
    ImGui::Text("Processing...");
    ImGui::Separator();
    
    float progress = m_progress.load();
    ImGui::ProgressBar(progress, ImVec2(-1, 0));
    
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        ImGui::Text("Stage: %s", m_progressStage.c_str());
    }
    
    if (ImGui::Button("Cancel")) {
        cancelProcessing();
    }
}

void AutoSurfaceDialog::renderResultsSection() {
    ImGui::Text("Results");
    ImGui::Separator();
    
    if (m_resultQuadMesh) {
        ImGui::Text("Quad Mesh: %d vertices, %d faces",
                   m_resultQuadMesh->vertexCount(), m_resultQuadMesh->faceCount());
    }
    
    if (!m_resultSurfaces.empty()) {
        ImGui::Text("NURBS Surfaces: %zu patches", m_resultSurfaces.size());
    }
    
    ImGui::Text("Processing Time: %.1f ms", m_metrics.processingTimeMs);
    ImGui::Text("Max Deviation: %.4f", m_metrics.maxDeviation);
    ImGui::Text("Avg Deviation: %.4f", m_metrics.averageDeviation);
    
    ImGui::Checkbox("Show Preview", &m_showPreview);
}

void AutoSurfaceDialog::renderButtons() {
    bool canProcess = m_inputMesh && !m_isProcessing;
    bool hasResults = m_resultQuadMesh != nullptr;
    
    if (!canProcess) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Generate", ImVec2(100, 0))) {
        startProcessing();
    }
    if (!canProcess) {
        ImGui::EndDisabled();
    }
    
    ImGui::SameLine();
    
    if (!hasResults) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Apply", ImVec2(100, 0))) {
        applyResults();
    }
    if (!hasResults) {
        ImGui::EndDisabled();
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Close", ImVec2(100, 0))) {
        hide();
    }
}

void AutoSurfaceDialog::startProcessing() {
    if (!m_inputMesh || m_isProcessing) return;
    
    // Validate parameters
    std::string error;
    if (!AutoSurfaceUtils::validateParameters(m_params, error)) {
        // Show error
        return;
    }
    
    m_isProcessing = true;
    m_progress = 0.0f;
    m_progressStage = "Starting...";
    
    // Set up progress callback
    m_autoSurface->setProgressCallback([this](float p, const std::string& stage) {
        onProgressUpdate(p, stage);
    });
    
    // Start processing thread
    m_processingThread = std::thread(&AutoSurfaceDialog::processAsync, this);
}

void AutoSurfaceDialog::cancelProcessing() {
    if (m_isProcessing && m_autoSurface) {
        m_autoSurface->cancel();
    }
}

void AutoSurfaceDialog::processAsync() {
    try {
        if (m_params.generateNurbs) {
            m_resultSurfaces = m_autoSurface->generateSurfaces(*m_inputMesh, m_params);
        } else {
            m_resultQuadMesh = m_autoSurface->generateQuadMesh(*m_inputMesh, m_params);
        }
    } catch (const std::exception& e) {
        // Handle error
    }
    
    m_isProcessing = false;
}

void AutoSurfaceDialog::onProgressUpdate(float progress, const std::string& stage) {
    m_progress = progress;
    
    std::lock_guard<std::mutex> lock(m_progressMutex);
    m_progressStage = stage;
}

void AutoSurfaceDialog::applyResults() {
    if (!m_resultQuadMesh && m_resultSurfaces.empty()) return;
    
    // Apply results to scene
    // This would typically create new scene objects or modify existing ones
    
    if (m_resultCallback) {
        m_resultCallback(true, "Results applied");
    }
    
    hide();
}

std::unique_ptr<QuadMesh> AutoSurfaceDialog::getQuadMeshResult() {
    return std::move(m_resultQuadMesh);
}

std::vector<std::unique_ptr<NurbsSurface>> AutoSurfaceDialog::getSurfaceResults() {
    return std::move(m_resultSurfaces);
}

// AutoSurfaceWizard implementation

AutoSurfaceWizard::AutoSurfaceWizard() {
    m_autoSurface = std::make_unique<AutoSurface>();
}

AutoSurfaceWizard::~AutoSurfaceWizard() {
    if (m_processingThread.joinable()) {
        m_autoSurface->cancel();
        m_processingThread.join();
    }
}

void AutoSurfaceWizard::show() {
    m_visible = true;
    m_currentStep = 0;
    updateParamsFromPreset();
}

void AutoSurfaceWizard::hide() {
    m_visible = false;
}

void AutoSurfaceWizard::setInputMesh(std::shared_ptr<TriangleMesh> mesh) {
    m_inputMesh = mesh;
}

void AutoSurfaceWizard::render() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver,
                           ImVec2(0.5f, 0.5f));
    
    if (ImGui::Begin("Auto Surface Wizard", &m_visible, 
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
        renderStepIndicator();
        ImGui::Separator();
        
        // Render current step
        switch (m_currentStep) {
            case 0: renderStep0_Introduction(); break;
            case 1: renderStep1_Quality(); break;
            case 2: renderStep2_Preview(); break;
            case 3: renderStep3_Complete(); break;
        }
        
        ImGui::Separator();
        renderNavigationButtons();
    }
    ImGui::End();
}

void AutoSurfaceWizard::update(float deltaTime) {
    // Check if processing completed
    if (m_processingThread.joinable() && !m_isProcessing) {
        m_processingThread.join();
        m_resultQuadMesh = m_autoSurface->getQuadMesh();
    }
}

void AutoSurfaceWizard::renderStepIndicator() {
    const char* stepNames[] = { "Welcome", "Quality", "Preview", "Complete" };
    
    ImGui::Spacing();
    
    for (int i = 0; i < STEP_COUNT; ++i) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled(">");
            ImGui::SameLine();
        }
        
        if (i == m_currentStep) {
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "%s", stepNames[i]);
        } else if (i < m_currentStep) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "%s", stepNames[i]);
        } else {
            ImGui::TextDisabled("%s", stepNames[i]);
        }
    }
    
    ImGui::Spacing();
}

void AutoSurfaceWizard::renderStep0_Introduction() {
    ImGui::TextWrapped(
        "This wizard will help you convert your triangle mesh into a clean quad mesh "
        "suitable for subdivision surface modeling and NURBS conversion.");
    
    ImGui::Spacing();
    
    if (m_inputMesh) {
        ImGui::Text("Input mesh:");
        ImGui::BulletText("%d vertices", m_inputMesh->vertexCount());
        ImGui::BulletText("%d triangles", m_inputMesh->triangleCount());
        
        // Estimate complexity
        int complexity = m_inputMesh->triangleCount();
        if (complexity < 1000) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Low complexity - fast processing expected");
        } else if (complexity < 10000) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Medium complexity - moderate processing time");
        } else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "High complexity - may take a while");
        }
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Please select an input mesh first.");
    }
}

void AutoSurfaceWizard::renderStep1_Quality() {
    ImGui::Text("Choose quality preset:");
    ImGui::Spacing();
    
    int preset = static_cast<int>(m_qualityPreset);
    
    if (ImGui::RadioButton("Draft (Fast)", preset == 0)) {
        m_qualityPreset = QualityPreset::Draft;
        updateParamsFromPreset();
    }
    ImGui::SetItemTooltip("Quick preview, lower quality");
    
    if (ImGui::RadioButton("Standard (Balanced)", preset == 1)) {
        m_qualityPreset = QualityPreset::Standard;
        updateParamsFromPreset();
    }
    ImGui::SetItemTooltip("Good balance of quality and speed");
    
    if (ImGui::RadioButton("High Quality (Slow)", preset == 2)) {
        m_qualityPreset = QualityPreset::High;
        updateParamsFromPreset();
    }
    ImGui::SetItemTooltip("Best quality, longer processing time");
    
    if (ImGui::RadioButton("Custom", preset == 3)) {
        m_qualityPreset = QualityPreset::Custom;
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Show sliders (disabled for non-custom presets)
    bool isCustom = (m_qualityPreset == QualityPreset::Custom);
    
    if (!isCustom) ImGui::BeginDisabled();
    
    ImGui::SliderFloat("Detail Level", &m_detailLevel, 0.0f, 1.0f, "%.2f");
    ImGui::SetItemTooltip("Higher = more detail/patches");
    
    ImGui::SliderFloat("Feature Sharpness", &m_featureSharpness, 0.0f, 1.0f, "%.2f");
    ImGui::SetItemTooltip("Higher = preserve more sharp edges");
    
    if (!isCustom) ImGui::EndDisabled();
    
    ImGui::Spacing();
    
    ImGui::Checkbox("Generate NURBS surfaces", &m_generateNurbs);
}

void AutoSurfaceWizard::renderStep2_Preview() {
    if (m_isProcessing) {
        ImGui::Text("Generating preview...");
        ImGui::ProgressBar(m_progress.load());
        
        if (ImGui::Button("Cancel")) {
            m_autoSurface->cancel();
        }
    } else if (m_resultQuadMesh) {
        ImGui::Text("Preview generated!");
        
        auto quality = m_resultQuadMesh->computeQuality();
        
        ImGui::Text("Result statistics:");
        ImGui::BulletText("%d quad faces", m_resultQuadMesh->faceCount());
        ImGui::BulletText("%.0f%% quads", quality.quadPercentage);
        ImGui::BulletText("%d irregular vertices", quality.irregularVertices);
        
        // Show quality rating
        float rating = quality.quadPercentage / 100.0f * 
                      (1.0f - static_cast<float>(quality.irregularVertices) / m_resultQuadMesh->vertexCount());
        
        ImGui::Spacing();
        if (rating > 0.8f) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Quality: Excellent");
        } else if (rating > 0.6f) {
            ImGui::TextColored(ImVec4(0.5f, 1, 0, 1), "Quality: Good");
        } else if (rating > 0.4f) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Quality: Fair");
        } else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Quality: Poor - try different settings");
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("Regenerate Preview")) {
            generatePreview();
        }
    } else {
        ImGui::Text("Click 'Generate Preview' to see the result.");
        
        if (ImGui::Button("Generate Preview")) {
            generatePreview();
        }
    }
}

void AutoSurfaceWizard::renderStep3_Complete() {
    ImGui::TextWrapped("Your quad mesh is ready!");
    ImGui::Spacing();
    
    if (m_resultQuadMesh) {
        ImGui::Text("Final result:");
        ImGui::BulletText("%d vertices", m_resultQuadMesh->vertexCount());
        ImGui::BulletText("%d faces", m_resultQuadMesh->faceCount());
        
        ImGui::Spacing();
        ImGui::TextWrapped(
            "Click 'Finish' to add the quad mesh to your scene. "
            "You can then edit it with the Freeform tool."
        );
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), 
                          "No result generated. Please go back and generate a preview first.");
    }
}

void AutoSurfaceWizard::renderNavigationButtons() {
    bool canGoBack = m_currentStep > 0 && !m_isProcessing;
    bool canGoNext = m_currentStep < STEP_COUNT - 1 && !m_isProcessing;
    bool canFinish = m_currentStep == STEP_COUNT - 1 && m_resultQuadMesh && !m_isProcessing;
    
    if (!canGoBack) ImGui::BeginDisabled();
    if (ImGui::Button("< Back")) {
        prevStep();
    }
    if (!canGoBack) ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    // Spacer
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 180, 0));
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel")) {
        hide();
    }
    
    ImGui::SameLine();
    
    if (m_currentStep < STEP_COUNT - 1) {
        if (!canGoNext) ImGui::BeginDisabled();
        if (ImGui::Button("Next >")) {
            nextStep();
        }
        if (!canGoNext) ImGui::EndDisabled();
    } else {
        if (!canFinish) ImGui::BeginDisabled();
        if (ImGui::Button("Finish")) {
            finalize();
        }
        if (!canFinish) ImGui::EndDisabled();
    }
}

void AutoSurfaceWizard::nextStep() {
    if (m_currentStep < STEP_COUNT - 1) {
        m_currentStep++;
        
        // Auto-generate preview when entering preview step
        if (m_currentStep == 2 && !m_resultQuadMesh) {
            generatePreview();
        }
    }
}

void AutoSurfaceWizard::prevStep() {
    if (m_currentStep > 0) {
        m_currentStep--;
    }
}

void AutoSurfaceWizard::generatePreview() {
    if (!m_inputMesh || m_isProcessing) return;
    
    updateParamsFromPreset();
    
    m_isProcessing = true;
    m_progress = 0.0f;
    
    m_autoSurface->setProgressCallback([this](float p, const std::string&) {
        m_progress = p;
    });
    
    m_processingThread = std::thread([this]() {
        m_resultQuadMesh = m_autoSurface->generateQuadMesh(*m_inputMesh, m_params);
        m_isProcessing = false;
    });
}

void AutoSurfaceWizard::finalize() {
    // Add result to scene and close
    hide();
}

void AutoSurfaceWizard::updateParamsFromPreset() {
    switch (m_qualityPreset) {
        case QualityPreset::Draft:
            m_params.targetPatchCount = 50;
            m_params.maxIterations = 20;
            m_params.deviationTolerance = 0.05f;
            m_params.featurePreservation = 0.3f;
            m_params.targetContinuity = 0;
            break;
            
        case QualityPreset::Standard:
            m_params.targetPatchCount = 100;
            m_params.maxIterations = 50;
            m_params.deviationTolerance = 0.01f;
            m_params.featurePreservation = 0.6f;
            m_params.targetContinuity = 1;
            break;
            
        case QualityPreset::High:
            m_params.targetPatchCount = 200;
            m_params.maxIterations = 100;
            m_params.deviationTolerance = 0.005f;
            m_params.featurePreservation = 0.9f;
            m_params.targetContinuity = 2;
            break;
            
        case QualityPreset::Custom:
            // Use slider values
            m_params.targetPatchCount = static_cast<int>(50 + m_detailLevel * 200);
            m_params.featurePreservation = m_featureSharpness;
            break;
    }
    
    m_params.generateNurbs = m_generateNurbs;
}

} // namespace dc
