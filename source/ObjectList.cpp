#include "ObjectList.hpp"

#include "CollectibleView.hpp"
#include "PoiInfo.hpp"
#include "PoiRegions.hpp"

#include <algorithm>
#include <cmath>

namespace gtasa {
namespace {

float distanceSquared(const ObjectListItem& item, float x, float y) {
    const float dx = item.x - x;
    const float dy = item.y - y;
    return dx * dx + dy * dy;
}

int categoryOrder(const ObjectListItem& item) {
    return item.kind == ObjectListKind::Collectible
        ? static_cast<int>(item.collectibleType)
        : static_cast<int>(CollectibleType::Count) + static_cast<int>(item.poiCategory);
}

} // namespace

FavoriteId favoriteIdForListItem(const ObjectListItem& item) {
    return item.kind == ObjectListKind::Poi
        ? FavoriteId{FavoriteKind::Poi, CollectibleType::Tag, item.id}
        : FavoriteId{FavoriteKind::Collectible, item.collectibleType, item.id};
}

std::vector<ObjectListItem> buildObjectList(const ParseResult& result, const ObjectListOptions& options,
                                            const Favorites& favorites) {
    std::vector<ObjectListItem> items;
    for (std::size_t index = 0; index < result.objects.size(); ++index) {
        const auto& item = result.objects[index];
        if (item.id <= 0 || !options.collectibleFilters[static_cast<std::size_t>(item.type)] ||
            !collectibleMatchesView(result, item, options.collectibleViewMode)) continue;
        const auto region = regionForCollectible(item.type, item.id);
        if (region == SanAndreasRegion::Count || !regionEnabled(options.regionFilters, region)) continue;
        ObjectListItem listItem{ObjectListKind::Collectible, item.type, item.id, static_cast<int>(index), region,
                                PoiCategory::Story, item.x, item.y, item.completed, false};
        listItem.favorite = favorites.contains(favoriteIdForListItem(listItem));
        if (!options.favoritesOnly || listItem.favorite) items.push_back(listItem);
    }
    if (options.showPoi) for (std::size_t index = 0; index < poiInfoCount(); ++index) {
        const auto* poi = poiInfo(index);
        if (!poi || !poi->visibleOnMap || !poiCategoryEnabled(options.poiCategoryFilters, poi->category)) continue;
        const auto region = regionForPoi(poi->id);
        if (!regionEnabled(options.regionFilters, region)) continue;
        ObjectListItem listItem{ObjectListKind::Poi, CollectibleType::Tag, poi->id, static_cast<int>(index), region,
                                poi->category, poi->x, poi->y, false, false};
        listItem.favorite = favorites.contains(favoriteIdForListItem(listItem));
        if (!options.favoritesOnly || listItem.favorite) items.push_back(listItem);
    }
    sortObjectList(items, ObjectListSort::Id, options.cursorX, options.cursorY);
    return items;
}

void sortObjectList(std::vector<ObjectListItem>& items, ObjectListSort sort, float cursorX, float cursorY) {
    std::stable_sort(items.begin(), items.end(), [&](const ObjectListItem& left, const ObjectListItem& right) {
        const auto tieBreak = [&] {
            if (left.kind != right.kind) return left.kind < right.kind;
            if (left.collectibleType != right.collectibleType) return left.collectibleType < right.collectibleType;
            return left.id < right.id;
        };
        if (sort == ObjectListSort::Id) return tieBreak();
        if (sort == ObjectListSort::Category) {
            if (categoryOrder(left) != categoryOrder(right)) return categoryOrder(left) < categoryOrder(right);
            return tieBreak();
        }
        if (sort == ObjectListSort::Region) {
            if (left.region != right.region) return left.region < right.region;
            return tieBreak();
        }
        const float leftDistance = distanceSquared(left, cursorX, cursorY);
        const float rightDistance = distanceSquared(right, cursorX, cursorY);
        if (leftDistance != rightDistance) return leftDistance < rightDistance;
        return tieBreak();
    });
}

} // namespace gtasa
