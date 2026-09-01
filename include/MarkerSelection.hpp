#pragma once

#include <cmath>
#include <vector>

namespace gtasa {

struct MarkerSelectionPoint { int id{}; float x{}; float y{}; };

inline bool cycleOverlappingMarker(const std::vector<MarkerSelectionPoint>& points, int currentId,
                                   int direction, float groupRadius, int& nextId) {
    const MarkerSelectionPoint* current = nullptr;
    for (const auto& point : points) if (point.id == currentId) { current = &point; break; }
    if (!current) return false;
    std::vector<int> group;
    const float limit = groupRadius * groupRadius;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const float dx = points[i].x - current->x, dy = points[i].y - current->y;
        if (dx * dx + dy * dy <= limit) group.push_back(static_cast<int>(i));
    }
    if (group.size() < 2) return false;
    int position = 0;
    for (std::size_t i = 0; i < group.size(); ++i) if (points[group[i]].id == currentId) position = static_cast<int>(i);
    const int count = static_cast<int>(group.size());
    nextId = points[group[(position + (direction < 0 ? count - 1 : 1)) % count]].id;
    return nextId != currentId;
}

} // namespace gtasa
