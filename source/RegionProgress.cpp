#include "RegionProgress.hpp"

#include "CollectibleInfo.hpp"
#include "CollectibleView.hpp"

namespace gtasa {

SanAndreasRegion regionForWorldPoint(float x, float y) {
    // These broad, mutually exclusive areas are intentionally based only on
    // canonical world coordinates. They avoid inventing fine-grained zone
    // membership while keeping all 320 collectibles in one readable region.
    if (y < -700.0f) return SanAndreasRegion::LosSantos;
    if (x < -1000.0f && y >= 0.0f) return SanAndreasRegion::SanFierro;
    if (x > 650.0f && y >= 450.0f) return SanAndreasRegion::LasVenturas;
    return SanAndreasRegion::Countryside;
}

const char* sanAndreasRegionName(SanAndreasRegion region, bool russian) {
    switch (region) {
        case SanAndreasRegion::LosSantos: return "Los Santos";
        case SanAndreasRegion::SanFierro: return "San Fierro";
        case SanAndreasRegion::LasVenturas: return "Las Venturas";
        case SanAndreasRegion::Countryside: return russian ? "Сельская местность" : "Countryside";
        default: return russian ? "Неизвестно" : "Unknown";
    }
}

std::array<RegionProgress, kSanAndreasRegionCount> calculateRegionProgress(const ParseResult& result) {
    std::array<RegionProgress, kSanAndreasRegionCount> progress{};
    for (std::size_t i = 0; i < collectibleInfoCount(); ++i) {
        // The generated catalogue is ordered by type/canonical ID. Look up the
        // record through the public API rather than relying on table layout.
        const CollectibleInfo* info = nullptr;
        for (const auto type : {CollectibleType::Tag, CollectibleType::Snapshot, CollectibleType::Horseshoe,
                                CollectibleType::Oyster, CollectibleType::StuntJump}) {
            const int typeOffset = type == CollectibleType::Tag ? 0 : type == CollectibleType::Snapshot ? 100 :
                                   type == CollectibleType::Horseshoe ? 150 : type == CollectibleType::Oyster ? 200 : 250;
            const int typeCount = type == CollectibleType::Tag ? 100 : type == CollectibleType::StuntJump ? 70 : 50;
            if (static_cast<int>(i) >= typeOffset && static_cast<int>(i) < typeOffset + typeCount) {
                info = collectibleInfo(type, static_cast<int>(i) - typeOffset + 1);
                break;
            }
        }
        if (!info) continue;
        auto& stats = progress[static_cast<std::size_t>(regionForWorldPoint(info->x, info->y))];
        ++stats.total;
        if (!collectibleCategoryHasReliableCompleted(result, info->type)) {
            ++stats.completionUnknown;
            continue;
        }
        for (const auto& item : result.objects) {
            if (item.type == info->type && item.id == info->canonicalId) {
                if (item.completed) ++stats.completed;
                break;
            }
        }
    }
    return progress;
}

} // namespace gtasa
