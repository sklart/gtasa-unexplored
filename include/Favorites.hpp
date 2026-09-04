#pragma once

#include "Collectibles.hpp"

#include <set>
#include <string>

namespace gtasa {

enum class FavoriteKind { Collectible, Poi };

struct FavoriteId {
    FavoriteKind kind{FavoriteKind::Collectible};
    CollectibleType type{CollectibleType::Tag};
    int id{};

    bool operator<(const FavoriteId& other) const;
};

class Favorites {
public:
    bool contains(FavoriteId id) const;
    bool toggle(FavoriteId id);
    std::string encode() const;
    bool decode(const std::string& encoded);
    std::size_t size() const { return values_.size(); }

private:
    std::set<FavoriteId> values_;
};

} // namespace gtasa
