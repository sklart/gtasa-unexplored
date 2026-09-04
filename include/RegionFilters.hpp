#pragma once

#include "RegionProgress.hpp"

#include <array>
#include <string>

namespace gtasa {

using RegionFilters = std::array<bool, kSanAndreasRegionCount>;

bool regionEnabled(const RegionFilters& filters, SanAndreasRegion region);
std::string encodeRegionFilters(const RegionFilters& filters);
bool decodeRegionFilters(const std::string& encoded, RegionFilters& filters);

// POI do not belong to the collectible catalogue. Their map filter uses a
// coarse geographic area solely for display grouping; collectible regions are
// always resolved by regionForCollectible().
SanAndreasRegion regionForPoiCoordinate(float x, float y);

} // namespace gtasa
