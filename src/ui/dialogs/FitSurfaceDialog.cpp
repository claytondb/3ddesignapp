#include "FitSurfaceDialog.h"
#include "../../geometry/mesh/TriangleMesh.h"
#include "../../geometry/nurbs/NurbsSurface.h"
#include "../../geometry/nurbs/NurbsCurve.h"
#include "../../scene/Selection.h"
#include "../imgui/imgui.h"

namespace dc {

FitSurfaceDialog::FitSurfaceDialog() {
    m_fitter = std::make_unique<SurfaceFitter>();
}

FitSurfaceDialog::~FitSurfaceDialog() {
    cancelFitting();
    if (m_processingThread.joinable()) {
        m_processingThread.join();
    }
}

void FitSurfaceDialog::show() {
    m_visible = true;
}

void FitSurfaceDialog::hide() {
    m_visible = false;
    cancelFitting();
}

void FitSurfaceDialog::setInputMesh(std::shared_ptr<TriangleMesh> mesh) {
    m_inputMesh = mesh;
    m_resultSurface.reset();
}

void FitSurfaceDialog::setSelectedFaces(const std::vector<int>& faceIndices) {
    m_selectedFaces = faceIndices;
    m_resultSurface.reset();
}

void FitSurfaceDialog::setBoundaryCurves(const std::vector<std::shared_ptr<NurbsCurve>>& curves) {
    m_boundaryCurves = curves;
}

void FitSurfaceDialog::render() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(420, 550), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Fit Surface", &m_visible, ImGuiWindowFlags_NoCollapse)) {
        if (m_isProcessing) {
            // Progress display
            ImGui::Text("Fitting surface...");
            ImGui::ProgressBar(m_progress.load());
            ImGui::Text("Current deviation: %.6f", m_currentDeviation.load());
            
            if (ImGui::Button("Cancel")) {
                cancelFitting();
            }
        } else {
            // Input summary
            renderRegionSection();
            ImGui::Separator();
            
            // Parameters in collapsing sections
            if (ImGui::CollapsingHeader("Surface Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
                renderSurfaceSection();
            }
            
            if (ImGui::CollapsingHeader("Tolerance", ImGuiTreeNodeFlags_DefaultOpen)) {
                renderToleranceSection();
            }
            
            if (ImGui::CollapsingHeader("Boundary Conditions")) {
                renderBoundarySection();
            }
            
            if (ImGui::CollapsingHeader("Advanced")) {
                renderAdvancedSection();
            }
            
            // Results
            if (m_resultSurface) {
                ImGui::Separator();
                renderResultsSection();
            }
            
            ImGui::Separator();
            renderButtons();
        }
    }
    ImGui::End();
}

void FitSurfaceDialog::update(float deltaTime) {
    if (m_processingThread.joinable() && !m_isProcessing) {
        m_processingThread.join();
        
        if (m_fitResult.surface) {
            m_resultSurface = std::move(m_fitResult.surface);
        }
        
        if (m_resultCallback) {
            m_resultCallback(m_resultSurface != nullptr);
        }
    }
}

void FitSurfaceDialog::renderRegionSection() {
    ImGui::Text("Fit Region");
    
    if (m_inputMesh) {
        ImGui::Text("Mesh: %d vertices, %d triangles",
                   m_inputMesh->vertexCount(), m_inputMesh->triangleCount());
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "No input mesh");
    }
    
    if (!m_selectedFaces.empty()) {
        ImGui::Text("Selected: %zu faces", m_selectedFaces.size());
    } else {
        ImGui::Text("Selection: Entire mesh");
    }
    
    if (!m_boundaryCurves.empty()) {
        ImGui::Text("Boundary curves: %zu", m_boundaryCurves.size());
    }
}

void FitSurfaceDialog::renderSurfaceSection() {
    ImGui::SliderInt("U Degree", &m_params.uDegree, 2, 5);
    ImGui::SliderInt("V Degree", &m_params.vDegree, 2, 5);
    
    ImGui::Spacing();
    
    ImGui::SliderInt("U Control Points", &m_params.uControlPoints, 4, 30);
    ImGui::SliderInt("V Control Points", &m_params.vControlPoints, 4, 30);
    
    // Quick presets
    ImGui::Spacing();
    ImGui::Text("Presets:");
    ImGui::SameLine();
    if (ImGui::SmallButton("Simple")) {
        m_params.uDegree = 3;
        m_params.vDegree = 3;
        m_params.uControlPoints = 6;
        m_params.vControlPoints = 6;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Detailed")) {
        m_params.uDegree = 3;
        m_params.vDegree = 3;
        m_params.uControlPoints = 12;
        m_params.vControlPoints = 12;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Complex")) {
        m_params.uDegree = 4;
        m_params.vDegree = 4;
        m_params.uControlPoints = 20;
        m_params.vControlPoints = 20;
    }
}

void FitSurfaceDialog::renderToleranceSection() {
    // Show in millimeters for easier understanding
    float devMm = m_params.deviationTolerance * 1000.0f;
    if (ImGui::SliderFloat("Max Deviation (mm)", &devMm, 0.001f, 10.0f, "%.3f")) {
        m_params.deviationTolerance = devMm / 1000.0f;
    }
    ImGui::SetItemTooltip("Target maximum distance from original surface");
    
    ImGui::SliderInt("Max Iterations", &m_params.maxIterations, 10, 500);
    
    float convThreshold = m_params.convergenceThreshold * 1000.0f;
    if (ImGui::SliderFloat("Convergence (x1000)", &convThreshold, 0.001f, 1.0f, "%.4f")) {
        m_params.convergenceThreshold = convThreshold / 1000.0f;
    }
    
    ImGui::Checkbox("Adaptive Refinement", &m_params.adaptiveRefinement);
    if (m_params.adaptiveRefinement) {
        ImGui::SliderInt("Max Refinement Level", &m_params.maxRefinementLevel, 1, 5);
    }
}

void FitSurfaceDialog::renderBoundarySection() {
    const char* conditionNames[] = { "Free", "Position", "Tangent (G1)", "Curvature (G2)", "Fixed" };
    const char* edgeNames[] = { "U Min (Start)", "U Max (End)", "V Min (Start)", "V Max (End)" };
    
    for (int i = 0; i < 4; ++i) {
        ImGui::PushID(i);
        
        if (ImGui::TreeNode(edgeNames[i])) {
            ImGui::Combo("Condition", &m_boundaryUI[i].conditionType, conditionNames, 5);
            
            if (!m_boundaryCurves.empty()) {
                ImGui::Checkbox("Use boundary curve", &m_boundaryUI[i].useCurve);
                
                if (m_boundaryUI[i].useCurve) {
                    // Create curve names for combo
                    std::vector<std::string> curveNames;
                    for (size_t j = 0; j < m_boundaryCurves.size(); ++j) {
                        curveNames.push_back("Curve " + std::to_string(j + 1));
                    }
                    
                    if (ImGui::BeginCombo("Curve", 
                        m_boundaryUI[i].curveIndex >= 0 ? curveNames[m_boundaryUI[i].curveIndex].c_str() : "Select...")) {
                        for (size_t j = 0; j < curveNames.size(); ++j) {
                            if (ImGui::Selectable(curveNames[j].c_str(), 
                                                  m_boundaryUI[i].curveIndex == static_cast<int>(j))) {
                                m_boundaryUI[i].curveIndex = static_cast<int>(j);
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
            }
            
            ImGui::TreePop();
        }
        
        ImGui::PopID();
    }
    
    // Apply UI state to params
    m_params.uMinCondition = boundaryConditionFromUI(0);
    m_params.uMaxCondition = boundaryConditionFromUI(1);
    m_params.vMinCondition = boundaryConditionFromUI(2);
    m_params.vMaxCondition = boundaryConditionFromUI(3);
}

void FitSurfaceDialog::renderAdvancedSection() {
    ImGui::SliderFloat("Smoothing Weight", &m_params.smoothingWeight, 0.0f, 1.0f);
    ImGui::SetItemTooltip("Higher values produce smoother surfaces with potentially more deviation");
    
    ImGui::SliderFloat("Fairing Weight", &m_params.fairingWeight, 0.0f, 0.1f, "%.4f");
    ImGui::SetItemTooltip("Minimizes surface energy (bending)");
}

void FitSurfaceDialog::renderResultsSection() {
    ImGui::Text("Fit Results");
    
    ImGui::Text("Status: %s", m_fitResult.converged ? "Converged" : "Max iterations reached");
    ImGui::Text("Iterations: %d", m_fitResult.iterations);
    
    ImGui::Spacing();
    
    ImGui::Text("Deviation:");
    ImGui::BulletText("Maximum: %.6f mm", m_fitResult.maxDeviation * 1000.0f);
    ImGui::BulletText("Average: %.6f mm", m_fitResult.averageDeviation * 1000.0f);
    ImGui::BulletText("RMS: %.6f mm", m_fitResult.rmsDeviation * 1000.0f);
    
    // Quality indicator
    float quality = 1.0f - std::min(1.0f, m_fitResult.maxDeviation / m_params.deviationTolerance);
    
    ImGui::Spacing();
    ImGui::Text("Quality:");
    ImGui::SameLine();
    
    if (quality > 0.9f) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Excellent");
    } else if (quality > 0.7f) {
        ImGui::TextColored(ImVec4(0.5f, 1, 0, 1), "Good");
    } else if (quality > 0.5f) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Fair");
    } else {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Below tolerance");
    }
    
    ImGui::Spacing();
    
    ImGui::Checkbox("Show Preview", &m_showPreview);
    ImGui::Checkbox("Show Deviation Map", &m_showDeviation);
}

void FitSurfaceDialog::renderButtons() {
    bool canFit = m_inputMesh && !m_isProcessing;
    bool hasResult = m_resultSurface != nullptr;
    
    if (!canFit) ImGui::BeginDisabled();
    if (ImGui::Button("Fit Surface", ImVec2(100, 0))) {
        startFitting();
    }
    if (!canFit) ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    if (!hasResult) ImGui::BeginDisabled();
    if (ImGui::Button("Apply", ImVec2(80, 0))) {
        applyResult();
    }
    if (!hasResult) ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    if (ImGui::Button("Close", ImVec2(80, 0))) {
        hide();
    }
}

void FitSurfaceDialog::startFitting() {
    if (!m_inputMesh || m_isProcessing) return;
    
    m_isProcessing = true;
    m_progress = 0.0f;
    m_currentDeviation = 0.0f;
    m_resultSurface.reset();
    
    m_fitter->setProgressCallback([this](float p, float dev) {
        onProgressUpdate(p, dev);
    });
    
    m_processingThread = std::thread(&FitSurfaceDialog::processAsync, this);
}

void FitSurfaceDialog::cancelFitting() {
    if (m_isProcessing && m_fitter) {
        m_fitter->cancel();
    }
}

void FitSurfaceDialog::processAsync() {
    try {
        if (m_selectedFaces.empty()) {
            // Fit to entire mesh - extract points
            std::vector<glm::vec3> points;
            std::vector<glm::vec3> normals;
            
            const auto& vertices = m_inputMesh->vertices();
            for (const auto& v : vertices) {
                points.push_back(v.position);
                normals.push_back(v.normal);
            }
            
            if (!m_boundaryCurves.empty() && m_boundaryCurves.size() >= 4) {
                m_fitResult = m_fitter->fitWithBoundaryCurves(points, m_boundaryCurves, m_params);
            } else {
                m_fitResult = m_fitter->fitToPointsWithNormals(points, normals, m_params);
            }
        } else {
            // Fit to selected region
            m_fitResult = m_fitter->fitToMeshRegion(*m_inputMesh, m_selectedFaces, m_params);
        }
    } catch (const std::exception& e) {
        m_fitResult.message = e.what();
        m_fitResult.converged = false;
    }
    
    m_isProcessing = false;
}

void FitSurfaceDialog::onProgressUpdate(float progress, float deviation) {
    m_progress = progress;
    m_currentDeviation = deviation;
}

void FitSurfaceDialog::applyResult() {
    if (!m_resultSurface) return;
    
    // Add surface to scene
    // This would typically be handled by a command/action system
    
    hide();
}

std::unique_ptr<NurbsSurface> FitSurfaceDialog::getResult() {
    return std::move(m_resultSurface);
}

BoundaryCondition FitSurfaceDialog::boundaryConditionFromUI(int index) const {
    static const BoundaryCondition mapping[] = {
        BoundaryCondition::Free,
        BoundaryCondition::Position,
        BoundaryCondition::Tangent,
        BoundaryCondition::Curvature,
        BoundaryCondition::Fixed
    };
    
    int type = m_boundaryUI[index].conditionType;
    if (type >= 0 && type < 5) {
        return mapping[type];
    }
    return BoundaryCondition::Free;
}

const char* FitSurfaceDialog::boundaryConditionName(BoundaryCondition bc) const {
    switch (bc) {
        case BoundaryCondition::Free: return "Free";
        case BoundaryCondition::Position: return "Position";
        case BoundaryCondition::Tangent: return "Tangent";
        case BoundaryCondition::Curvature: return "Curvature";
        case BoundaryCondition::Fixed: return "Fixed";
    }
    return "Unknown";
}

// CurveNetworkFitDialog implementation

CurveNetworkFitDialog::CurveNetworkFitDialog() {
    m_fitter = std::make_unique<SurfaceFitter>();
}

CurveNetworkFitDialog::~CurveNetworkFitDialog() {
    if (m_processingThread.joinable()) {
        m_fitter->cancel();
        m_processingThread.join();
    }
}

void CurveNetworkFitDialog::show() {
    m_visible = true;
    if (m_autoClassify && !m_curves.empty()) {
        autoClassifyCurves();
    }
}

void CurveNetworkFitDialog::hide() {
    m_visible = false;
}

void CurveNetworkFitDialog::setCurves(const std::vector<std::shared_ptr<NurbsCurve>>& curves) {
    m_curves = curves;
    m_uCurveIndices.clear();
    m_vCurveIndices.clear();
    
    if (m_autoClassify) {
        autoClassifyCurves();
    }
}

void CurveNetworkFitDialog::render() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Fit Surface to Curve Network", &m_visible)) {
        if (m_isProcessing) {
            ImGui::Text("Fitting surface...");
            if (ImGui::Button("Cancel")) {
                m_fitter->cancel();
            }
        } else {
            renderCurveList();
            ImGui::Separator();
            
            renderClassificationSection();
            ImGui::Separator();
            
            renderParameterSection();
            ImGui::Separator();
            
            renderButtons();
        }
    }
    ImGui::End();
}

void CurveNetworkFitDialog::update(float deltaTime) {
    if (m_processingThread.joinable() && !m_isProcessing) {
        m_processingThread.join();
    }
}

void CurveNetworkFitDialog::renderCurveList() {
    ImGui::Text("Input Curves: %zu", m_curves.size());
    
    if (m_curves.size() < 2) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Need at least 2 curves");
    }
}

void CurveNetworkFitDialog::renderClassificationSection() {
    ImGui::Text("Curve Direction");
    
    ImGui::Checkbox("Auto-classify", &m_autoClassify);
    
    if (m_autoClassify) {
        if (ImGui::Button("Re-classify")) {
            autoClassifyCurves();
        }
    }
    
    ImGui::Text("U curves: %zu, V curves: %zu", 
               m_uCurveIndices.size(), m_vCurveIndices.size());
    
    // Manual classification UI would go here
}

void CurveNetworkFitDialog::renderParameterSection() {
    ImGui::SliderInt("Surface Degree", &m_params.uDegree, 2, 5);
    m_params.vDegree = m_params.uDegree; // Keep same
    
    float devMm = m_params.deviationTolerance * 1000.0f;
    if (ImGui::SliderFloat("Tolerance (mm)", &devMm, 0.001f, 1.0f)) {
        m_params.deviationTolerance = devMm / 1000.0f;
    }
}

void CurveNetworkFitDialog::renderButtons() {
    bool canFit = m_curves.size() >= 2 && !m_isProcessing;
    
    if (!canFit) ImGui::BeginDisabled();
    if (ImGui::Button("Fit Surface")) {
        startFitting();
    }
    if (!canFit) ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    if (ImGui::Button("Close")) {
        hide();
    }
}

void CurveNetworkFitDialog::autoClassifyCurves() {
    m_uCurveIndices.clear();
    m_vCurveIndices.clear();
    
    if (m_curves.empty()) return;
    
    // Simple heuristic: use direction of first curve as U,
    // curves perpendicular to it are V
    
    glm::vec3 firstDir = glm::normalize(
        m_curves[0]->evaluate(1.0f) - m_curves[0]->evaluate(0.0f)
    );
    
    for (size_t i = 0; i < m_curves.size(); ++i) {
        glm::vec3 dir = glm::normalize(
            m_curves[i]->evaluate(1.0f) - m_curves[i]->evaluate(0.0f)
        );
        
        float dot = std::abs(glm::dot(dir, firstDir));
        
        if (dot > 0.7f) {
            m_uCurveIndices.push_back(static_cast<int>(i));
        } else {
            m_vCurveIndices.push_back(static_cast<int>(i));
        }
    }
}

void CurveNetworkFitDialog::startFitting() {
    if (m_curves.size() < 2 || m_isProcessing) return;
    
    m_isProcessing = true;
    
    m_processingThread = std::thread([this]() {
        auto result = m_fitter->fitToCurveNetwork(m_curves, m_params);
        m_resultSurface = std::move(result.surface);
        m_isProcessing = false;
    });
}

std::unique_ptr<NurbsSurface> CurveNetworkFitDialog::getResult() {
    return std::move(m_resultSurface);
}

// QuickFitSurfaceDialog implementation

QuickFitSurfaceDialog::QuickFitSurfaceDialog() {
    m_fitter = std::make_unique<SurfaceFitter>();
}

QuickFitSurfaceDialog::~QuickFitSurfaceDialog() = default;

void QuickFitSurfaceDialog::show() {
    m_visible = true;
}

void QuickFitSurfaceDialog::hide() {
    m_visible = false;
}

void QuickFitSurfaceDialog::setSelection(std::shared_ptr<Selection> selection) {
    m_selection = selection;
}

void QuickFitSurfaceDialog::render() {
    if (!m_visible) return;
    
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Quick Fit Surface", &m_visible)) {
        renderQualitySelector();
        ImGui::Separator();
        renderPreview();
        
        ImGui::Separator();
        
        if (!m_isProcessing) {
            if (ImGui::Button("Fit", ImVec2(80, 0))) {
                startFitting();
            }
            ImGui::SameLine();
        }
        
        if (ImGui::Button("Close", ImVec2(80, 0))) {
            hide();
        }
    }
    ImGui::End();
}

void QuickFitSurfaceDialog::update(float deltaTime) {
    // Check processing state
}

void QuickFitSurfaceDialog::renderQualitySelector() {
    ImGui::Text("Quality:");
    
    if (ImGui::RadioButton("Low (Fast)", m_quality == 0)) m_quality = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Medium", m_quality == 1)) m_quality = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("High", m_quality == 2)) m_quality = 2;
    
    ImGui::SliderInt("Degree", &m_degree, 2, 5);
    
    ImGui::Checkbox("Match boundaries", &m_matchBoundaries);
}

void QuickFitSurfaceDialog::renderPreview() {
    if (m_result) {
        ImGui::Text("Surface generated");
        // Show preview stats
    }
}

void QuickFitSurfaceDialog::startFitting() {
    if (!m_selection || m_isProcessing) return;
    
    SurfaceFitParams params;
    params.uDegree = m_degree;
    params.vDegree = m_degree;
    
    switch (m_quality) {
        case 0:
            params.uControlPoints = 6;
            params.vControlPoints = 6;
            params.maxIterations = 20;
            break;
        case 1:
            params.uControlPoints = 10;
            params.vControlPoints = 10;
            params.maxIterations = 50;
            break;
        case 2:
            params.uControlPoints = 16;
            params.vControlPoints = 16;
            params.maxIterations = 100;
            break;
    }
    
    // Get points from selection and fit
    // ...
}

} // namespace dc
