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
        if (!reliable) {
            // Pickup coordinates in result.missing come directly from the
            // save. They remain safe to show even when a canonical identity
            // cannot be recovered. Only the complementary Completed set is
            // unknown, so never manufacture it from an unreliable mapping.
            allReliable = false;
            for (const auto& raw : result.missing) {
                if (raw.type != type) continue;
                Collectible unclassified = raw;
                // The raw point is safe to render, but its sequential pickup
                // ordinal is not a canonical Wiki identity.
                unclassified.id = 0;
                result.objects.push_back(unclassified);
            }
            continue;
        }
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

bool collectibleCategoryHasReliableCompleted(const ParseResult& result, CollectibleType type) {
    switch (type) {
        case CollectibleType::Snapshot: return result.snapshotsCatalogueReliable;
        case CollectibleType::Horseshoe: return result.horseshoesCatalogueReliable;
        case CollectibleType::Oyster: return result.oystersCatalogueReliable;
        case CollectibleType::Tag:
        case CollectibleType::StuntJump: return true;
        default: return false;
    }
}

bool collectibleMatchesView(const ParseResult& result, const Collectible& item, CollectibleViewMode mode) {
    if (mode == CollectibleViewMode::Missing) return !item.completed;
    const bool completedAvailable = collectibleCategoryHasReliableCompleted(result, item.type);
    if (mode == CollectibleViewMode::Completed) return completedAvailable && item.completed;
    // In All mode an unreliable category deliberately contains only its raw,
    // trustworthy Missing entries. Its unavailable Completed complement is not
    // silently represented by guessed points.
    return !item.completed || completedAvailable;
}

} // namespace gtasa
