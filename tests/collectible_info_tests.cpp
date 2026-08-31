#include "CollectibleInfo.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace gtasa;

namespace {
bool close(float a, float b) {
    return std::fabs(a - b) < 0.02f;
}

void assertWorld(CollectibleType type, int wikiId, float x, float y, float z) {
    const auto* info = collectibleInfo(type, wikiId);
    assert(info);
    assert(close(info->x, x) && close(info->y, y) && close(info->z, z));
    Collectible runtime{type, 0, x, y, z, false, false, 0};
    const auto* resolved = collectibleInfoForRuntime(runtime);
    assert(resolved && resolved->canonicalId == wikiId);
}
} // namespace

int main() {
    assert(collectibleInfoCount() == 320);

    // Tags: Wiki/catalog identity and save-array identity are independent.
    Collectible tag{CollectibleType::Tag, 38, 2046.4f, -1635.8f, 13.6f, false, false, 37};
    const auto* tagInfo = collectibleInfoForRuntime(tag);
    assert(tagInfo && tagInfo->canonicalId == 1 && tagInfo->tagSaveOrderId == 38);

    // Regression anchors for every non-Tag category. These fail under the old equal-ID join.
    assertWorld(CollectibleType::Snapshot, 1, -964.53f, -331.59f, 47.16f);    // catalog #34
    assertWorld(CollectibleType::Horseshoe, 1, 984.0f, 2563.0f, 12.0f);       // catalog #29
    assertWorld(CollectibleType::Oyster, 1, 2179.0f, 235.0f, -5.0f);          // catalog #50, true Z
    assertWorld(CollectibleType::StuntJump, 1, 2460.18f, -2567.91f, 18.82f);  // catalog #7

    // Oyster high-Z regression: the old z=0 builder cannot match this pickup in 3D.
    assertWorld(CollectibleType::Oyster, 12, 1279.0f, -806.0f, 85.0f);        // catalog #3

    // Stunt order regression: Wiki #70 is raw catalog #43, not #70.
    assertWorld(CollectibleType::StuntJump, 70, 1749.72f, 779.60f, 13.48f);

    // Quantisation tolerance remains supported for normal pickups.
    const auto* snapshot = collectibleInfo(CollectibleType::Snapshot, 1);
    Collectible quantised{CollectibleType::Snapshot, 0, snapshot->x + 0.125f,
                          snapshot->y - 0.125f, snapshot->z, false, false, 0};
    assert(collectibleInfoForRuntime(quantised)->canonicalId == 1);
    quantised.x += 100.0f;
    assert(collectibleInfoForRuntime(quantised) == nullptr);

    std::cout << "collectible metadata tests passed\n";
}
