#include "CollectibleInfo.hpp"

#include <cassert>
#include <iostream>

using namespace gtasa;

int main() {
    assert(collectibleInfoCount() == 320);
    // Canonical Wiki Tag #1 is save-order entry 38, not entry 1.
    Collectible tag{CollectibleType::Tag, 38, 2046.4f, -1635.8f, 13.6f, false, false, 37};
    const auto* tagInfo = collectibleInfoForRuntime(tag);
    assert(tagInfo && tagInfo->canonicalId == 1 && tagInfo->tagSaveOrderId == 38);

    const auto* snapshot = collectibleInfo(CollectibleType::Snapshot, 1);
    assert(snapshot);
    Collectible quantised{CollectibleType::Snapshot, 0, snapshot->x + 0.125f,
                          snapshot->y - 0.125f, snapshot->z, false, false, 0};
    assert(collectibleInfoForRuntime(quantised)->canonicalId == 1);
    quantised.x += 100.0f;
    assert(collectibleInfoForRuntime(quantised) == nullptr);
    std::cout << "collectible metadata tests passed\n";
}
