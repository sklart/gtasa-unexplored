#include "PoiInfo.hpp"
#include "PoiRegions.hpp"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    std::array<int, kSanAndreasRegionCount> counts{};
    assert(poiInfoCount() == 80);
    for (int id = 1; id <= 80; ++id) {
        const auto region = regionForPoi(id);
        assert(region != SanAndreasRegion::Count);
        ++counts[static_cast<std::size_t>(region)];
    }
    assert((counts == std::array<int, kSanAndreasRegionCount>{14, 14, 14, 38}));
    assert(regionForPoi(0) == SanAndreasRegion::Count);
    assert(regionForPoi(81) == SanAndreasRegion::Count);
    std::cout << "POI region tests passed\n";
}
