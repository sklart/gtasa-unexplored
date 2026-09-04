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

    // An unmappable pickup must remain available as the exact raw Missing
    // point. Its unknown Completed complement is fail-closed.
    ParseResult unreliable;
    unreliable.ok = true;
    unreliable.missing.push_back({CollectibleType::Snapshot, 1, 2999.0f, 2999.0f, 0.0f, false, false, 0});
    assert(!buildCollectibleObjects(unreliable));
    assert(!unreliable.snapshotsCatalogueReliable);
    assert(collectibleCategoryHasReliableCompleted(unreliable, CollectibleType::Tag));
    assert(!collectibleCategoryHasReliableCompleted(unreliable, CollectibleType::Snapshot));
    int rawSnapshotCount = 0;
    for (const auto& item : unreliable.objects) {
        if (item.type != CollectibleType::Snapshot) continue;
        ++rawSnapshotCount;
        assert(!item.completed);
        assert(collectibleMatchesView(unreliable, item, CollectibleViewMode::Missing));
        assert(!collectibleMatchesView(unreliable, item, CollectibleViewMode::Completed));
        assert(collectibleMatchesView(unreliable, item, CollectibleViewMode::All));
    }
    assert(rawSnapshotCount == 1);
    std::cout << "collectible view tests passed\n";
}
