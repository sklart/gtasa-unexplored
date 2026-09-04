#include "CollectibleView.hpp"

#include "CollectibleInfo.hpp"

#include <array>
#include <set>

namespace gtasa {
namespace {

int totalForType(CollectibleType type) {
    switch (type) {
        case CollectibleType::Tag: return 100;
        case CollectibleType::Snapshot: return 50;
        case CollectibleType::Horseshoe: return 50;
        case CollectibleType::Oyster: return 50;
        case CollectibleType::StuntJump: return 70;
        default: return 0;
    }
}

bool setReliable(ParseResult& result, CollectibleType type, bool reliable) {
    switch (type) {
        case CollectibleType::Snapshot: result.snapshotsCatalogueReliable = reliable; break;
        case CollectibleType::Horseshoe: result.horseshoesCatalogueReliable = reliable; break;
        case CollectibleType::Oyster: result.oystersCatalogueReliable = reliable; break;
        default: break;
    }
    return reliable;
}

} // namespace

bool buildCollectibleObjects(ParseResult& result) {
    result.objects.clear();
    if (!result.ok) return false;
    bool allReliable = true;
    for (const auto type : {CollectibleType::Tag, CollectibleType::Snapshot, CollectibleType::Horseshoe,
                            CollectibleType::Oyster, CollectibleType::StuntJump}) {
        std::set<int> missingIds;
        bool reliable = true;
        for (const auto& missing : result.missing) {
            if (missing.type != type) continue;
            const auto* info = collectibleInfoForRuntime(missing);
            if (!info || !missingIds.insert(info->canonicalId).second) { reliable = false; break; }
        }
        const int total = totalForType(type);
        if (static_cast<int>(missingIds.size()) > total) reliable = false;
        setReliable(result, type, reliable);
        if (!reliable) { allReliable = false; continue; }
        for (int id = 1; id <= total; ++id) {
            const auto* info = collectibleInfo(type, id);
            if (!info) { allReliable = false; break; }
            const bool missing = missingIds.count(id) != 0;
            bool found = false;
            if (missing) for (const auto& raw : result.missing) {
                if (raw.type == type) {
                    const auto* rawInfo = collectibleInfoForRuntime(raw);
                    if (rawInfo && rawInfo->canonicalId == id) { found = raw.found; break; }
                }
            }
            result.objects.push_back({type, id, info->x, info->y, info->z, !missing, found, id - 1});
        }
    }
    return allReliable;
}

bool collectibleMatchesView(const Collectible& item, CollectibleViewMode mode) {
    switch (mode) {
        case CollectibleViewMode::Missing: return !item.completed;
        case CollectibleViewMode::Completed: return item.completed;
        case CollectibleViewMode::All: return true;
    }
    return false;
}

} // namespace gtasa
