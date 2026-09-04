#include "RegionProgress.hpp"

#include "CollectibleView.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <utility>

int main() {
    using namespace gtasa;
    constexpr std::array<int, kSanAndreasRegionCount> kExpectedTotals{149, 59, 70, 42};
    constexpr std::array<std::pair<CollectibleType, int>, 5> kTypes{{
        {CollectibleType::Tag, 100}, {CollectibleType::Snapshot, 50},
        {CollectibleType::Horseshoe, 50}, {CollectibleType::Oyster, 50},
        {CollectibleType::StuntJump, 70},
    }};

    std::array<int, kSanAndreasRegionCount> catalogueTotals{};
    int classified = 0;
    for (const auto& [type, count] : kTypes) {
        for (int canonicalId = 1; canonicalId <= count; ++canonicalId) {
            const auto region = regionForCollectible(type, canonicalId);
            assert(region != SanAndreasRegion::Count);
            ++catalogueTotals[static_cast<std::size_t>(region)];
            ++classified;
        }
    }
    assert(classified == 320);
    assert(catalogueTotals == kExpectedTotals);

    ParseResult result;
    result.ok = true;
    assert(buildCollectibleObjects(result));
    const auto progress = calculateRegionProgress(result);
    int total = 0;
    int completed = 0;
    int unknown = 0;
    for (const auto& stats : progress) {
        total += stats.total;
        completed += stats.completed;
        unknown += stats.completionUnknown;
    }
    assert(total == 320);
    assert(completed == 320);
    assert(unknown == 0);
    for (std::size_t i = 0; i < kSanAndreasRegionCount; ++i) assert(progress[i].total == kExpectedTotals[i]);
    std::cout << "region progress tests passed\n";
}
