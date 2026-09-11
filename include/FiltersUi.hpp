#pragma once
#include "Collectibles.hpp"
#include "PoiCategories.hpp"
#include "RegionProgress.hpp"
namespace gtasa {
struct FiltersUiLayout { int collectibleFirst; int regionFirst; int poiRow; int poiCategoryFirst; int modeRow; int count; };
// The global Missing/Completed/All mode comes first: it is not a POI filter.
constexpr FiltersUiLayout filtersUiLayout() {
    const int m = 0;
    const int c = m + 1;
    const int r = c + static_cast<int>(CollectibleType::Count);
    const int p = r + static_cast<int>(kSanAndreasRegionCount);
    const int pc = p + 1;
    return {c, r, p, pc, m, pc + static_cast<int>(kPoiCategoryCount)};
}
int nextFilterRow(int current, int direction);
} // namespace gtasa
