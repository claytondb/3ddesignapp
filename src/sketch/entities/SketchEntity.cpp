#include "SketchEntity.h"

namespace dc {
namespace sketch {

// FIX: Thread-safe ID generation using atomic
std::atomic<uint64_t> SketchEntity::s_nextId{1};

SketchEntity::SketchEntity(SketchEntityType type)
    : m_id(s_nextId++)
    , m_type(type)
{
}

std::vector<glm::vec2> SketchEntity::tessellate(int numSamples) const {
    std::vector<glm::vec2> points;
    points.reserve(numSamples + 1);
    
    for (int i = 0; i <= numSamples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSamples);
        points.push_back(evaluate(t));
    }
    
    return points;
}

} // namespace sketch
} // namespace dc
