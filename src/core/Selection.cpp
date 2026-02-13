/**
 * @file Selection.cpp
 * @brief Implementation of the Selection manager
 * 
 * Thread Safety Note: Selection is designed to be accessed from the main
 * (UI) thread only. All signal/slot connections should use Qt::AutoConnection
 * to ensure thread safety when signals cross thread boundaries.
 */

#include "Selection.h"
#include <algorithm>

namespace dc3d {
namespace core {

Selection::Selection(QObject* parent)
    : QObject(parent)
{
}

void Selection::setMode(SelectionMode mode)
{
    if (m_mode != mode) {
        // Clear selection when changing modes
        if (!m_selectedElements.empty()) {
            m_selectedElements.clear();
            emit selectionChanged();
        }
        
        m_mode = mode;
        emit modeChanged(mode);
    }
}

void Selection::select(const SelectionElement& element, SelectionOp op)
{
    bool changed = false;
    
    switch (op) {
        case SelectionOp::Replace:
            // Only skip if we already have exactly this one element selected
            if (m_selectedElements.size() != 1 || m_selectedElements.find(element) == m_selectedElements.end()) {
                m_selectedElements.clear();
                m_selectedElements.insert(element);
                changed = true;
            }
            break;
            
        case SelectionOp::Add:
            if (m_selectedElements.insert(element).second) {
                changed = true;
            }
            break;
            
        case SelectionOp::Toggle:
            {
                auto it = m_selectedElements.find(element);
                if (it != m_selectedElements.end()) {
                    m_selectedElements.erase(it);
                } else {
                    m_selectedElements.insert(element);
                }
                changed = true;
            }
            break;
            
        case SelectionOp::Remove:
            if (m_selectedElements.erase(element) > 0) {
                changed = true;
            }
            break;
    }
    
    if (changed) {
        emit selectionChanged();
    }
}

void Selection::select(const std::vector<SelectionElement>& elements, SelectionOp op)
{
    if (elements.empty()) {
        if (op == SelectionOp::Replace && !m_selectedElements.empty()) {
            m_selectedElements.clear();
            emit selectionChanged();
        }
        return;
    }
    
    bool changed = false;
    
    switch (op) {
        case SelectionOp::Replace:
            m_selectedElements.clear();
            for (const auto& elem : elements) {
                m_selectedElements.insert(elem);
            }
            changed = true;
            break;
            
        case SelectionOp::Add:
            for (const auto& elem : elements) {
                if (m_selectedElements.insert(elem).second) {
                    changed = true;
                }
            }
            break;
            
        case SelectionOp::Toggle:
            for (const auto& elem : elements) {
                auto it = m_selectedElements.find(elem);
                if (it != m_selectedElements.end()) {
                    m_selectedElements.erase(it);
                } else {
                    m_selectedElements.insert(elem);
                }
            }
            changed = true;
            break;
            
        case SelectionOp::Remove:
            for (const auto& elem : elements) {
                if (m_selectedElements.erase(elem) > 0) {
                    changed = true;
                }
            }
            break;
    }
    
    if (changed) {
        emit selectionChanged();
    }
}

SelectionElement Selection::createElementFromHit(const HitInfo& hit) const
{
    SelectionElement elem;
    elem.meshId = hit.meshId;
    elem.mode = m_mode;
    
    switch (m_mode) {
        case SelectionMode::Object:
            elem.elementIndex = 0;  // Not used for object mode
            break;
            
        case SelectionMode::Face:
            elem.elementIndex = hit.faceIndex;
            break;
            
        case SelectionMode::Vertex:
            elem.elementIndex = hit.closestVertex;
            break;
            
        case SelectionMode::Edge:
            // Validate closestEdge before using it as an index
            {
                uint32_t v1, v2;
                if (hit.closestEdge >= 0 && hit.closestEdge <= 2) {
                    // Valid edge index
                    if (hit.closestEdge == 0) {
                        v1 = hit.vertexIndices[0];
                        v2 = hit.vertexIndices[1];
                    } else if (hit.closestEdge == 1) {
                        v1 = hit.vertexIndices[1];
                        v2 = hit.vertexIndices[2];
                    } else {
                        v1 = hit.vertexIndices[2];
                        v2 = hit.vertexIndices[0];
                    }
                } else {
                    // Invalid edge index (-1 or out of range), default to edge 0
                    v1 = hit.vertexIndices[0];
                    v2 = hit.vertexIndices[1];
                }
                // Ensure consistent ordering
                if (v1 > v2) std::swap(v1, v2);
                // Use 64-bit encoding: upper 32 bits = v2, lower 32 bits = v1
                elem.elementIndex = (static_cast<uint64_t>(v2) << 32) | static_cast<uint64_t>(v1);
            }
            break;
    }
    
    return elem;
}

void Selection::selectFromHit(const HitInfo& hit, SelectionOp op)
{
    if (!hit.hit) {
        // Clicked on nothing
        if (op == SelectionOp::Replace) {
            clear();
        }
        return;
    }
    
    SelectionElement elem = createElementFromHit(hit);
    select(elem, op);
}

void Selection::deselect(const SelectionElement& element)
{
    if (m_selectedElements.erase(element) > 0) {
        emit selectionChanged();
    }
}

void Selection::clear()
{
    if (!m_selectedElements.empty()) {
        m_selectedElements.clear();
        emit selectionChanged();
    }
}

void Selection::invertSelection(uint32_t meshId, uint32_t totalElements)
{
    std::set<SelectionElement> newSelection;
    
    for (uint32_t i = 0; i < totalElements; ++i) {
        SelectionElement elem;
        elem.meshId = meshId;
        elem.elementIndex = i;
        elem.mode = m_mode;
        
        if (m_selectedElements.find(elem) == m_selectedElements.end()) {
            newSelection.insert(elem);
        }
    }
    
    // Keep elements from other meshes
    for (const auto& elem : m_selectedElements) {
        if (elem.meshId != meshId) {
            newSelection.insert(elem);
        }
    }
    
    if (newSelection != m_selectedElements) {
        m_selectedElements = std::move(newSelection);
        emit selectionChanged();
    }
}

void Selection::selectAll(uint32_t meshId, uint32_t totalElements)
{
    bool changed = false;
    
    for (uint32_t i = 0; i < totalElements; ++i) {
        SelectionElement elem;
        elem.meshId = meshId;
        elem.elementIndex = i;
        elem.mode = m_mode;
        
        if (m_selectedElements.insert(elem).second) {
            changed = true;
        }
    }
    
    if (changed) {
        emit selectionChanged();
    }
}

bool Selection::isSelected(const SelectionElement& element) const
{
    return m_selectedElements.find(element) != m_selectedElements.end();
}

bool Selection::hasSelection(uint32_t meshId) const
{
    for (const auto& elem : m_selectedElements) {
        if (elem.meshId == meshId) {
            return true;
        }
    }
    return false;
}

std::vector<uint32_t> Selection::selectedIndices(uint32_t meshId) const
{
    std::vector<uint32_t> indices;
    
    for (const auto& elem : m_selectedElements) {
        if (elem.meshId == meshId) {
            indices.push_back(elem.elementIndex);
        }
    }
    
    return indices;
}

std::vector<uint32_t> Selection::selectedMeshIds() const
{
    std::set<uint32_t> meshIds;
    
    for (const auto& elem : m_selectedElements) {
        meshIds.insert(elem.meshId);
    }
    
    return std::vector<uint32_t>(meshIds.begin(), meshIds.end());
}

void Selection::selectObject(uint32_t meshId, SelectionOp op)
{
    // Switch to Object mode if not already in it
    // This ensures the element mode matches the current selection mode
    if (m_mode != SelectionMode::Object) {
        setMode(SelectionMode::Object);
    }
    
    SelectionElement elem;
    elem.meshId = meshId;
    elem.elementIndex = 0;
    elem.mode = SelectionMode::Object;
    
    select(elem, op);
}

void Selection::deselectObject(uint32_t meshId)
{
    SelectionElement elem;
    elem.meshId = meshId;
    elem.elementIndex = 0;
    elem.mode = SelectionMode::Object;
    
    deselect(elem);
}

bool Selection::isObjectSelected(uint32_t meshId) const
{
    SelectionElement elem;
    elem.meshId = meshId;
    elem.elementIndex = 0;
    elem.mode = SelectionMode::Object;
    
    return isSelected(elem);
}

void Selection::selectFace(uint32_t meshId, uint32_t faceIndex, SelectionOp op)
{
    // Switch to Face mode if not already in it
    if (m_mode != SelectionMode::Face) {
        setMode(SelectionMode::Face);
    }
    
    SelectionElement elem;
    elem.meshId = meshId;
    elem.elementIndex = faceIndex;
    elem.mode = SelectionMode::Face;
    
    select(elem, op);
}

void Selection::selectVertex(uint32_t meshId, uint32_t vertexIndex, SelectionOp op)
{
    // Switch to Vertex mode if not already in it
    if (m_mode != SelectionMode::Vertex) {
        setMode(SelectionMode::Vertex);
    }
    
    SelectionElement elem;
    elem.meshId = meshId;
    elem.elementIndex = vertexIndex;
    elem.mode = SelectionMode::Vertex;
    
    select(elem, op);
}

void Selection::selectEdge(uint32_t meshId, uint32_t v1, uint32_t v2, SelectionOp op)
{
    // Switch to Edge mode if not already in it
    if (m_mode != SelectionMode::Edge) {
        setMode(SelectionMode::Edge);
    }
    
    // Ensure consistent ordering
    if (v1 > v2) std::swap(v1, v2);
    
    SelectionElement elem;
    elem.meshId = meshId;
    // Use 64-bit encoding: upper 32 bits = v2, lower 32 bits = v1 (supports large meshes)
    elem.elementIndex = (static_cast<uint64_t>(v2) << 32) | static_cast<uint64_t>(v1);
    elem.mode = SelectionMode::Edge;
    
    select(elem, op);
}

} // namespace core
} // namespace dc3d
