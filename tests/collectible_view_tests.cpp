#include "CollectibleView.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    ParseResult result;
    result.ok = true;
    assert(buildCollectibleObjects(result));
    assert(result.objects.size() == 320);
    for (const auto& item : result.objects) assert(item.completed);
    assert(collectibleMatchesView(result.objects.front(), CollectibleViewMode::Completed));
    assert(!collectibleMatchesView(result.objects.front(), CollectibleViewMode::Missing));
    result.objects.front().completed = false;
    assert(collectibleMatchesView(result.objects.front(), CollectibleViewMode::Missing));
    assert(collectibleMatchesView(result.objects.front(), CollectibleViewMode::All));
    std::cout << "collectible view tests passed\n";
}
