#pragma once

#include "CollectibleView.hpp"
#include "RegionFilters.hpp"

#include <array>

namespace gtasa {

int nearestMissingCollectibleIndex(const ParseResult& result, float originX, float originY,
                                   const std::array<bool, static_cast<int>(CollectibleType::Count)>& filters,
                                   const RegionFilters& regionFilters,
                                   CollectibleViewMode viewMode);

} // namespace gtasa
