#pragma once

#include "Collectibles.hpp"

struct SDL_Renderer;
struct SDL_Texture;

namespace gtasa {

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
