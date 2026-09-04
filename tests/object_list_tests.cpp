#include "ObjectList.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    ParseResult result;
    result.ok = true;
    assert(buildCollectibleObjects(result));
    assert(result.objects.size() == 320);
    result.objects[0].completed = false;
    result.objects[100].completed = false;

    Favorites favorites;
    ObjectListOptions options;
    options.showPoi = false;
    options.collectibleViewMode = CollectibleViewMode::All;
    options.regionFilters = {true, false, false, false};
    auto items = buildObjectList(result, options, favorites);
    assert(items.size() == 149);
    for (const auto& item : items) assert(item.region == SanAndreasRegion::LosSantos);

    options.regionFilters = {true, true, true, true};
    options.collectibleViewMode = CollectibleViewMode::Missing;
    items = buildObjectList(result, options, favorites);
    assert(items.size() == 2);
    for (const auto& item : items) assert(!item.completed);
    options.collectibleViewMode = CollectibleViewMode::Completed;
    items = buildObjectList(result, options, favorites);
    assert(items.size() == 318);
    for (const auto& item : items) assert(item.completed);
    options.collectibleViewMode = CollectibleViewMode::All;

    const auto favorite = favoriteIdForListItem(items.front());
    assert(favorites.toggle(favorite));
    options.favoritesOnly = true;
    items = buildObjectList(result, options, favorites);
    assert(items.size() == 1 && items.front().favorite);
    // sourceIndex plus world position are the complete, stable transition
    // target consumed by the map UI when A is pressed in the object list.
    assert(items.front().sourceIndex == 1);
    assert(items.front().x == result.objects[1].x && items.front().y == result.objects[1].y);

    std::vector<ObjectListItem> sortable{
        {ObjectListKind::Poi, CollectibleType::Tag, 3, 0, SanAndreasRegion::LasVenturas, PoiCategory::Nature, 5, 0},
        {ObjectListKind::Collectible, CollectibleType::StuntJump, 8, 1, SanAndreasRegion::Countryside, PoiCategory::Story, 20, 0},
        {ObjectListKind::Collectible, CollectibleType::Tag, 2, 2, SanAndreasRegion::LosSantos, PoiCategory::Story, 1, 0},
    };
    sortObjectList(sortable, ObjectListSort::Id, 0, 0);
    assert(sortable.front().collectibleType == CollectibleType::Tag && sortable.front().id == 2);
    sortObjectList(sortable, ObjectListSort::Category, 0, 0);
    assert(sortable.front().collectibleType == CollectibleType::Tag);
    sortObjectList(sortable, ObjectListSort::Region, 0, 0);
    assert(sortable.front().region == SanAndreasRegion::LosSantos);
    sortObjectList(sortable, ObjectListSort::Distance, 0, 0);
    assert(sortable.front().x == 1.0f && sortable.front().y == 0.0f);
    std::cout << "object list tests passed\n";
}
