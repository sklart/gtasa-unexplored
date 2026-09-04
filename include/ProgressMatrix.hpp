#pragma once

#include "RegionProgress.hpp"

#include <array>

namespace gtasa {

using ProgressMatrix = std::array<std::array<RegionProgress, static_cast<std::size_t>(CollectibleType::Count)>, kSanAndreasRegionCount>;
ProgressMatrix calculateProgressMatrix(const ParseResult& result);

} // namespace gtasa
