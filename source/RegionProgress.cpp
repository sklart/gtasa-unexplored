#include "RegionProgress.hpp"

#include "CollectibleView.hpp"

#include <array>

namespace gtasa {

namespace {

// Canonical ID order: Tags 1..100, Snapshots 1..50, Horseshoes 1..50,
// Oysters 1..50, Unique Stunt Jumps 1..70. L/S/V/C are Los Santos,
// San Fierro, Las Venturas and Countryside respectively. This is deliberately
// data, not a coordinate-derived heuristic: a collectible's region remains
// stable if a future coordinate correction changes its X/Y value.
constexpr char kRegionCatalogue[] =
    // Tags (1..100)
    "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
    // Snapshots (1..50)
    "CCCSLLCLCCCCSSCSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSCSSS"
    // Horseshoes (1..50)
    "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV"
    // Oysters (1..50)
    "CCCCLLLLLLLLLLLLLLLLLLLCSCSSSSSSSSSSSCCCCCVVVVVVVV"
    // Unique Stunt Jumps (1..70)
    "LLLLLLLLLLLLLLLLLLLLLLLLCCCCCCCCCCCLLLCCCCCSSSSSSSSSSCCCCCVVVVVVVVVVVV";

constexpr std::array<CollectibleType, 5> kTypes{
    CollectibleType::Tag, CollectibleType::Snapshot, CollectibleType::Horseshoe,
    CollectibleType::Oyster, CollectibleType::StuntJump,
};

constexpr int typeCount(CollectibleType type) {
    return type == CollectibleType::Tag ? 100 : type == CollectibleType::StuntJump ? 70 : 50;
}

constexpr int catalogueOffset(CollectibleType type) {
    return type == CollectibleType::Tag ? 0 : type == CollectibleType::Snapshot ? 100 :
           type == CollectibleType::Horseshoe ? 150 : type == CollectibleType::Oyster ? 200 :
           type == CollectibleType::StuntJump ? 250 : -1;
}

static_assert(sizeof(kRegionCatalogue) == 321, "The fixed region catalogue must cover all 320 collectibles");

} // namespace

SanAndreasRegion regionForCollectible(CollectibleType type, int canonicalId) {
    const int offset = catalogueOffset(type);
    if (offset < 0 || canonicalId < 1 || canonicalId > typeCount(type)) return SanAndreasRegion::Count;
    switch (kRegionCatalogue[offset + canonicalId - 1]) {
        case 'L': return SanAndreasRegion::LosSantos;
        case 'S': return SanAndreasRegion::SanFierro;
        case 'V': return SanAndreasRegion::LasVenturas;
        case 'C': return SanAndreasRegion::Countryside;
        default: return SanAndreasRegion::Count;
    }
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
    for (const auto type : kTypes) {
        for (int canonicalId = 1; canonicalId <= typeCount(type); ++canonicalId) {
            const auto region = regionForCollectible(type, canonicalId);
            if (region == SanAndreasRegion::Count) continue;
            auto& stats = progress[static_cast<std::size_t>(region)];
            ++stats.total;
            if (!collectibleCategoryHasReliableCompleted(result, type)) {
                ++stats.completionUnknown;
                continue;
            }
            for (const auto& item : result.objects) {
                if (item.type == type && item.id == canonicalId) {
                    if (item.completed) ++stats.completed;
                    break;
                }
            }
        }
    }
    return progress;
}

} // namespace gtasa
