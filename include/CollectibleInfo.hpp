#pragma once

#include "Collectibles.hpp"

#include <cstddef>

namespace gtasa {

struct CollectibleInfo {
    CollectibleType type;
    int canonicalId;
    float x, y, z;
    const char* descriptionEn;
    const char* descriptionRu;
    const char* imagePath; // Relative to sdmc:/switch/gtasa-unexplored/collectibles/.
    int tagSaveOrderId;
};

const CollectibleInfo* collectibleInfo(CollectibleType type, int canonicalId);
const CollectibleInfo* collectibleInfoForRuntime(const Collectible& collectible);
std::size_t collectibleInfoCount();

} // namespace gtasa
