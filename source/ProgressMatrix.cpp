#include "ProgressMatrix.hpp"

#include "CollectibleView.hpp"

namespace gtasa {
namespace {
constexpr std::array<CollectibleType, 5> kTypes{CollectibleType::Tag, CollectibleType::Snapshot,
    CollectibleType::Horseshoe, CollectibleType::Oyster, CollectibleType::StuntJump};
constexpr int countFor(CollectibleType type) { return type == CollectibleType::Tag ? 100 : type == CollectibleType::StuntJump ? 70 : 50; }
}

ProgressMatrix calculateProgressMatrix(const ParseResult& result) {
    ProgressMatrix matrix{};
    for (const auto type : kTypes) for (int id = 1; id <= countFor(type); ++id) {
        const auto region = regionForCollectible(type, id);
        auto& cell = matrix[static_cast<std::size_t>(region)][static_cast<std::size_t>(type)];
        ++cell.total;
        if (!collectibleCategoryHasReliableCompleted(result, type)) { ++cell.completionUnknown; continue; }
        for (const auto& item : result.objects) if (item.type == type && item.id == id) { if (item.completed) ++cell.completed; break; }
    }
    return matrix;
}

} // namespace gtasa
