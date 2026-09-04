#pragma once

#include "CollectibleView.hpp"

#include <array>

namespace gtasa {

int nearestMissingCollectibleIndex(const ParseResult& result, float originX, float originY,
                                   const std::array<bool, static_cast<int>(CollectibleType::Count)>& filters,
                                   CollectibleViewMode viewMode);

} // namespace gtasa
