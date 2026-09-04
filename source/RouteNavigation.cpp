#include "RouteNavigation.hpp"

namespace gtasa {
int normalizeRouteIndex(int index, int count) { return count > 0 ? (index % count + count) % count : 0; }
int nextRouteIndex(int current, int count) { return count > 0 ? normalizeRouteIndex(current + 1, count) : 0; }
std::vector<int> orderedRouteIndices(const std::vector<int>& route, int currentIndex) {
    std::vector<int> ordered;
    if (route.empty()) return ordered;
    ordered.reserve(route.size());
    const int start = normalizeRouteIndex(currentIndex, static_cast<int>(route.size()));
    for (int i = 0; i < static_cast<int>(route.size()); ++i)
        ordered.push_back(route[static_cast<std::size_t>((start + i) % static_cast<int>(route.size()))]);
    return ordered;
}
} // namespace gtasa
