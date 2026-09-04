#include "Favorites.hpp"

#include <sstream>

namespace gtasa {

bool FavoriteId::operator<(const FavoriteId& other) const {
    if (kind != other.kind) return kind < other.kind;
    if (type != other.type) return type < other.type;
    return id < other.id;
}

bool Favorites::contains(FavoriteId id) const {
    return values_.count(id) != 0;
}

bool Favorites::toggle(FavoriteId id) {
    if (id.id <= 0 || (id.kind == FavoriteKind::Collectible &&
                       static_cast<int>(id.type) >= static_cast<int>(CollectibleType::Count))) return false;
    const auto it = values_.find(id);
    if (it == values_.end()) { values_.insert(id); return true; }
    values_.erase(it);
    return false;
}

std::string Favorites::encode() const {
    std::ostringstream encoded;
    bool first = true;
    for (const auto& value : values_) {
        if (!first) encoded << ',';
        first = false;
        if (value.kind == FavoriteKind::Poi) encoded << 'p' << ':' << value.id;
        else encoded << 'c' << ':' << static_cast<int>(value.type) << ':' << value.id;
    }
    return encoded.str();
}

bool Favorites::decode(const std::string& encoded) {
    std::set<FavoriteId> decoded;
    if (encoded.empty()) { values_.clear(); return true; }
    std::istringstream input(encoded);
    std::string token;
    while (std::getline(input, token, ',')) {
        std::istringstream part(token);
        std::string kind, type, id, extra;
        if (!std::getline(part, kind, ':') || !std::getline(part, type, ':')) return false;
        try {
            if (kind == "p") {
                if (std::getline(part, extra, ':') || type.empty()) return false;
                const int poiId = std::stoi(type);
                if (poiId <= 0) return false;
                decoded.insert({FavoriteKind::Poi, CollectibleType::Tag, poiId});
            } else if (kind == "c") {
                if (!std::getline(part, id, ':') || std::getline(part, extra, ':')) return false;
                const int collectibleType = std::stoi(type);
                const int collectibleId = std::stoi(id);
                if (collectibleType < 0 || collectibleType >= static_cast<int>(CollectibleType::Count) || collectibleId <= 0)
                    return false;
                decoded.insert({FavoriteKind::Collectible, static_cast<CollectibleType>(collectibleType), collectibleId});
            } else return false;
        } catch (...) { return false; }
    }
    values_ = std::move(decoded);
    return true;
}

} // namespace gtasa
