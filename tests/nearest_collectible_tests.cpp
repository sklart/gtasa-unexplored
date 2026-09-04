#include "NearestCollectible.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    ParseResult result;
    result.ok = true;
    result.objects = {
        {CollectibleType::Tag, 1, 10.0f, 0.0f, 0.0f, false, false, 0},
        {CollectibleType::Snapshot, 1, 2.0f, 0.0f, 0.0f, false, false, 0},
        {CollectibleType::Horseshoe, 1, 1.0f, 0.0f, 0.0f, true, false, 0},
    };
    std::array<bool, static_cast<int>(CollectibleType::Count)> filters{true, true, true, true, true};
    RegionFilters regions{true, true, true, true};
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::All) == 1);
    filters[static_cast<std::size_t>(CollectibleType::Snapshot)] = false;
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::Missing) == 0);
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::Completed) == -1);
    filters[static_cast<std::size_t>(CollectibleType::Tag)] = false;
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::All) == -1);

    // Tag #1 is Los Santos and Snapshot #1 is Countryside in the fixed
    // canonical catalogue. The nearer Snapshot must not bypass region filters.
    filters[static_cast<std::size_t>(CollectibleType::Tag)] = true;
    filters[static_cast<std::size_t>(CollectibleType::Snapshot)] = true;
    regions = {true, false, false, false};
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::Missing) == 0);
    regions = {false, false, false, true};
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::Missing) == 1);
    regions = {false, false, false, false};
    assert(nearestMissingCollectibleIndex(result, 0.0f, 0.0f, filters, regions, CollectibleViewMode::Missing) == -1);
    std::cout << "nearest collectible tests passed\n";
}
