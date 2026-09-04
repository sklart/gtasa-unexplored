#pragma once

#include "CollectibleView.hpp"
#include "Favorites.hpp"
#include "PoiCategories.hpp"
#include "RegionFilters.hpp"

#include <array>
#include <vector>

namespace gtasa {

enum class ObjectListKind { Collectible, Poi };
enum class ObjectListSort { Id, Category, Region, Distance };

struct ObjectListItem {
    ObjectListKind kind{ObjectListKind::Collectible};
    CollectibleType collectibleType{CollectibleType::Tag};
    int id{};
    int sourceIndex{};
    SanAndreasRegion region{SanAndreasRegion::Count};
    PoiCategory poiCategory{PoiCategory::Story};
    float x{}, y{};
    bool completed{};
    bool favorite{};
};

struct ObjectListOptions {
    std::array<bool, static_cast<int>(CollectibleType::Count)> collectibleFilters{true, true, true, true, true};
    RegionFilters regionFilters{true, true, true, true};
    bool showPoi{true};
    PoiCategoryFilters poiCategoryFilters{true, true, true, true, true};
    CollectibleViewMode collectibleViewMode{CollectibleViewMode::Missing};
    bool favoritesOnly{};
    float cursorX{}, cursorY{};
};

std::vector<ObjectListItem> buildObjectList(const ParseResult& result, const ObjectListOptions& options,
                                            const Favorites& favorites);
void sortObjectList(std::vector<ObjectListItem>& items, ObjectListSort sort, float cursorX, float cursorY);
FavoriteId favoriteIdForListItem(const ObjectListItem& item);

} // namespace gtasa
