#pragma once

#include "Collectibles.hpp"

namespace gtasa {

enum class CollectibleViewMode { Missing = 0, Completed = 1, All = 2 };

bool buildCollectibleObjects(ParseResult& result);
bool collectibleMatchesView(const Collectible& item, CollectibleViewMode mode);
bool collectibleCategoryHasReliableCompleted(const ParseResult& result, CollectibleType type);
bool collectibleMatchesView(const ParseResult& result, const Collectible& item, CollectibleViewMode mode);

} // namespace gtasa
