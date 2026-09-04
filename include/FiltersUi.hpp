#pragma once
#include "Collectibles.hpp"
#include "PoiCategories.hpp"
#include "RegionProgress.hpp"
namespace gtasa {
struct FiltersUiLayout { int regionFirst; int poiRow; int poiCategoryFirst; int modeRow; int count; };
constexpr FiltersUiLayout filtersUiLayout() { const int r=static_cast<int>(CollectibleType::Count); const int p=r+static_cast<int>(kSanAndreasRegionCount); const int c=p+1; const int m=c+static_cast<int>(kPoiCategoryCount); return {r,p,c,m,m+1}; }
int nextFilterRow(int current, int direction);
} // namespace gtasa
