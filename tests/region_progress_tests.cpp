#include "RegionProgress.hpp"

#include "CollectibleView.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
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
    assert(regionForWorldPoint(2500.0f, -1700.0f) == SanAndreasRegion::LosSantos);
    assert(regionForWorldPoint(-1800.0f, 900.0f) == SanAndreasRegion::SanFierro);
    assert(regionForWorldPoint(1500.0f, 1500.0f) == SanAndreasRegion::LasVenturas);
    assert(regionForWorldPoint(0.0f, 0.0f) == SanAndreasRegion::Countryside);
    std::cout << "region progress tests passed\n";
}
