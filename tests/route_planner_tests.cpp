#include "RoutePlanner.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    ParseResult result;
    result.ok = true;
    // Tag #1/#2 are Los Santos; Snapshot #1 is Countryside.
    result.objects = {
        {CollectibleType::Tag, 1, 10, 0, 0, false},
        {CollectibleType::Tag, 2, 20, 0, 0, false},
        {CollectibleType::Snapshot, 1, 1, 0, 0, false},
        {CollectibleType::Horseshoe, 1, 2, 0, 0, true},
    };
    std::array<bool, static_cast<int>(CollectibleType::Count)> categories{true, true, true, true, true};
    RegionFilters all{true, true, true, true};
    const auto full = planMissingRoute(result, 0, 0, categories, all, 0);
    assert((full == std::vector<int>{2, 0, 1}));
    assert(planMissingRoute(result, 0, 0, categories, all, 5).size() == 3);
    assert(planMissingRoute(result, 0, 0, categories, all, 10).size() == 3);
    RegionFilters losSantos{true, false, false, false};
    const auto losRoute = planMissingRoute(result, 0, 0, categories, losSantos, 0);
    assert((losRoute == std::vector<int>{0, 1}));
    categories[static_cast<std::size_t>(CollectibleType::Tag)] = false;
    assert(planMissingRoute(result, 0, 0, categories, losSantos, 0).empty());
    std::cout << "route planner tests passed\n";
}
