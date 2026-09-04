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

// Every canonical collectible has an explicit, embedded regional assignment.
// Count is returned only for an invalid type/ID pair; valid catalogue entries
// are always assigned to exactly one of the four regions.
SanAndreasRegion regionForCollectible(CollectibleType type, int canonicalId);
const char* sanAndreasRegionName(SanAndreasRegion region, bool russian);
std::array<RegionProgress, kSanAndreasRegionCount> calculateRegionProgress(const ParseResult& result);

} // namespace gtasa
