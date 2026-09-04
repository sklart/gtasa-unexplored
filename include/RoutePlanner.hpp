#pragma once

#include "RegionFilters.hpp"

#include <array>
#include <vector>

namespace gtasa {

// Greedy world-distance route through Missing canonical collectibles. maxStops
// is 5, 10, or 0 for all matching stops.
std::vector<int> planMissingRoute(const ParseResult& result, float originX, float originY,
                                  const std::array<bool, static_cast<int>(CollectibleType::Count)>& categoryFilters,
                                  const RegionFilters& regionFilters, int maxStops);

} // namespace gtasa
