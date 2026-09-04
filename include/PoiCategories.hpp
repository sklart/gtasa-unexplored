#pragma once

#include "PoiInfo.hpp"

#include <array>
#include <string>

namespace gtasa {

constexpr std::size_t kPoiCategoryCount = static_cast<std::size_t>(PoiCategory::Count);
using PoiCategoryFilters = std::array<bool, kPoiCategoryCount>;

const char* poiCategoryName(PoiCategory category, bool russian);
bool poiCategoryEnabled(const PoiCategoryFilters& filters, PoiCategory category);
std::string encodePoiCategoryFilters(const PoiCategoryFilters& filters);
bool decodePoiCategoryFilters(const std::string& encoded, PoiCategoryFilters& filters);

} // namespace gtasa
