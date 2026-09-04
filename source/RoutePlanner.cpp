#include "RoutePlanner.hpp"

#include "CollectibleView.hpp"

#include <limits>

namespace gtasa {

std::vector<int> planMissingRoute(const ParseResult& result, float originX, float originY,
                                  const std::array<bool, static_cast<int>(CollectibleType::Count)>& categoryFilters,
                                  const RegionFilters& regionFilters, int maxStops) {
    std::vector<int> route;
    if (maxStops < 0) return route;
    float x = originX, y = originY;
    while (maxStops == 0 || static_cast<int>(route.size()) < maxStops) {
        float bestDistance = std::numeric_limits<float>::infinity();
        int best = -1;
        for (std::size_t i = 0; i < result.objects.size(); ++i) {
            const auto& item = result.objects[i];
            const auto type = static_cast<std::size_t>(item.type);
            if (item.id <= 0 || type >= categoryFilters.size() || !categoryFilters[type] || item.completed ||
                !collectibleMatchesView(result, item, CollectibleViewMode::Missing) ||
                !regionEnabled(regionFilters, regionForCollectible(item.type, item.id))) continue;
            bool alreadyAdded = false;
            for (const int routeIndex : route) if (routeIndex == static_cast<int>(i)) { alreadyAdded = true; break; }
            if (alreadyAdded) continue;
            const float dx = item.x - x, dy = item.y - y;
            const float d2 = dx * dx + dy * dy;
            if (d2 < bestDistance) { bestDistance = d2; best = static_cast<int>(i); }
        }
        if (best < 0) break;
        route.push_back(best);
        x = result.objects[static_cast<std::size_t>(best)].x;
        y = result.objects[static_cast<std::size_t>(best)].y;
    }
    return route;
}

} // namespace gtasa
