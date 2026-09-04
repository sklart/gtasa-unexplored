#include "PoiIcon.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include "poi_marker_bin.h"

namespace gtasa {

PoiIcon::~PoiIcon() { unload(); }

bool PoiIcon::load(SDL_Renderer* renderer) {
    unload();
    SDL_RWops* rw = SDL_RWFromConstMem(poi_marker_bin, static_cast<int>(poi_marker_bin_size));
    if (!rw) return false;
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) return false;
    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture_) SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    return texture_ != nullptr;
}

void PoiIcon::unload() {
    SDL_DestroyTexture(texture_);
    texture_ = nullptr;
}

} // namespace gtasa
