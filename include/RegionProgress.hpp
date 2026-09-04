#pragma once

#include "Collectibles.hpp"

#include <array>
#include <cstddef>

namespace gtasa {

enum class SanAndreasRegion { LosSantos, SanFierro, LasVenturas, Countryside, Count };
constexpr std::size_t kSanAndreasRegionCount = static_cast<std::size_t>(SanAndreasRegion::Count);

struct RegionProgress {
    int total{};
    int completed{};
    int completionUnknown{};
};

SanAndreasRegion regionForWorldPoint(float x, float y);
const char* sanAndreasRegionName(SanAndreasRegion region, bool russian);
std::array<RegionProgress, kSanAndreasRegionCount> calculateRegionProgress(const ParseResult& result);

} // namespace gtasa
