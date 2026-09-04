#include "Favorites.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    Favorites favorites;
    const FavoriteId tag{FavoriteKind::Collectible, CollectibleType::Tag, 38};
    const FavoriteId jump{FavoriteKind::Collectible, CollectibleType::StuntJump, 7};
    const FavoriteId poi{FavoriteKind::Poi, CollectibleType::Tag, 12};
    assert(favorites.toggle(tag));
    assert(favorites.toggle(jump));
    assert(favorites.toggle(poi));
    assert(favorites.contains(tag));
    assert(favorites.size() == 3);
    const auto encoded = favorites.encode();

    Favorites restored;
    assert(restored.decode(encoded));
    assert(restored.contains(tag));
    assert(restored.contains(jump));
    assert(restored.contains(poi));
    assert(!restored.toggle(tag));
    assert(!restored.contains(tag));
    const auto beforeMalformed = restored.encode();
    assert(!restored.decode("c:5:1"));
    assert(restored.encode() == beforeMalformed);
    assert(restored.decode(""));
    assert(restored.size() == 0);
    std::cout << "favorites tests passed\n";
}
