#pragma once

#include "Collectibles.hpp"

struct SDL_Renderer;
struct SDL_Texture;

namespace gtasa {

// Keep map symbols legible on the Switch display without tying their size to
// the map's world-space scale.
inline int collectibleIconSize(float zoom) {
    if (zoom <= 1.0f) return 28;
    if (zoom >= 8.0f) return 42;
    const float t = (zoom - 1.0f) / 7.0f;
    return 28 + static_cast<int>(t * 14.0f + 0.5f);
}

class CollectibleIcons {
public:
    ~CollectibleIcons();
    bool load(SDL_Renderer* renderer);
    void unload();
    SDL_Texture* texture(CollectibleType type) const;

private:
    SDL_Texture* textures_[static_cast<int>(CollectibleType::Count)]{};
};

} // namespace gtasa
