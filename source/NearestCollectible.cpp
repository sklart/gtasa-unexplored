#include "NearestCollectible.hpp"

#include <limits>

namespace gtasa {

int nearestMissingCollectibleIndex(const ParseResult& result, float originX, float originY,
                                   const std::array<bool, static_cast<int>(CollectibleType::Count)>& filters,
                                   CollectibleViewMode viewMode) {
    float bestDistance2 = std::numeric_limits<float>::infinity();
    int best = -1;
    for (std::size_t i = 0; i < result.objects.size(); ++i) {
        const auto& item = result.objects[i];
        const auto type = static_cast<std::size_t>(item.type);
        if (type >= filters.size() || !filters[type] || item.completed ||
            !collectibleMatchesView(result, item, viewMode)) continue;
        const float dx = item.x - originX;
        const float dy = item.y - originY;
        const float distance2 = dx * dx + dy * dy;
        if (distance2 < bestDistance2) {
            bestDistance2 = distance2;
            best = static_cast<int>(i);
        }
    }
    return best;
}

} // namespace gtasa
