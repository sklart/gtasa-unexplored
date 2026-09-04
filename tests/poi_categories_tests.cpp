#include "PoiCategories.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    assert(poiInfoCount() == 80);
    PoiCategoryFilters filters{true, true, true, true, true};
    int counts[kPoiCategoryCount]{};
    for (std::size_t i = 0; i < poiInfoCount(); ++i) {
        const auto* poi = poiInfo(i);
        assert(poi);
        const auto category = static_cast<std::size_t>(poi->category);
        assert(category < kPoiCategoryCount);
        ++counts[category];
        assert(poiCategoryEnabled(filters, poi->category));
    }
    assert(counts[static_cast<std::size_t>(PoiCategory::Story)] == 18);
    assert(counts[static_cast<std::size_t>(PoiCategory::Landmark)] == 43);
    assert(counts[static_cast<std::size_t>(PoiCategory::Nature)] == 9);
    assert(counts[static_cast<std::size_t>(PoiCategory::Mystery)] == 7);
    assert(counts[static_cast<std::size_t>(PoiCategory::Business)] == 3);
    filters[static_cast<std::size_t>(PoiCategory::Mystery)] = false;
    assert(!poiCategoryEnabled(filters, PoiCategory::Mystery));
    assert(poiCategoryEnabled(filters, PoiCategory::Story));
    const auto encoded = encodePoiCategoryFilters(filters);
    assert(encoded == "11101");
    PoiCategoryFilters restored{};
    assert(decodePoiCategoryFilters(encoded, restored));
    assert(restored == filters);
    assert(!decodePoiCategoryFilters("101", restored));
    assert(!decodePoiCategoryFilters("111x1", restored));
    std::cout << "POI category tests passed\n";
}
