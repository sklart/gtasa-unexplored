#pragma once

#include "RegionProgress.hpp"

namespace gtasa {

// Explicit geographic grouping for every POI identity (IDs 1..80).
SanAndreasRegion regionForPoi(int poiId);

} // namespace gtasa
