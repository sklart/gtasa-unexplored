#include "CollectibleIcons.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include "collectible_horseshoe_bin.h"
#include "collectible_oyster_bin.h"
#include "collectible_snapshot_bin.h"
#include "collectible_stunt_jump_bin.h"
#include "collectible_tag_bin.h"

namespace gtasa {
namespace {

SDL_Texture* loadEmbedded(SDL_Renderer* renderer, const unsigned char* data, std::size_t size) {
    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(size));
    if (!rw) return nullptr;
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

} // namespace

CollectibleIcons::~CollectibleIcons() { unload(); }

bool CollectibleIcons::load(SDL_Renderer* renderer) {
    unload();
    textures_[static_cast<int>(CollectibleType::Tag)] = loadEmbedded(renderer, collectible_tag_bin, collectible_tag_bin_size);
    textures_[static_cast<int>(CollectibleType::Snapshot)] = loadEmbedded(renderer, collectible_snapshot_bin, collectible_snapshot_bin_size);
    textures_[static_cast<int>(CollectibleType::Horseshoe)] = loadEmbedded(renderer, collectible_horseshoe_bin, collectible_horseshoe_bin_size);
    textures_[static_cast<int>(CollectibleType::Oyster)] = loadEmbedded(renderer, collectible_oyster_bin, collectible_oyster_bin_size);
    textures_[static_cast<int>(CollectibleType::StuntJump)] = loadEmbedded(renderer, collectible_stunt_jump_bin, collectible_stunt_jump_bin_size);
    for (SDL_Texture* texture : textures_) if (!texture) { unload(); return false; }
    return true;
}

void CollectibleIcons::unload() {
    for (auto& texture : textures_) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

SDL_Texture* CollectibleIcons::texture(CollectibleType type) const {
    const int index = static_cast<int>(type);
    return index >= 0 && index < static_cast<int>(CollectibleType::Count) ? textures_[index] : nullptr;
}

} // namespace gtasa
