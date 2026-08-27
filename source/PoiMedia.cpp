#include "PoiMedia.hpp"

#include "Platform.hpp"
#include "PoiInfo.hpp"

#include <SDL.h>
#include <SDL_image.h>

namespace gtasa {

PoiMedia::~PoiMedia() { unload(); }

bool PoiMedia::load(SDL_Renderer* renderer, const PoiInfo& info, std::string& error) {
    unload();
    error_.clear();
    const std::string path = std::string(kAppDir) + "/poi/" + info.imagePath;
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) { error = IMG_GetError(); error_ = error; return false; }
    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture_) { error = SDL_GetError(); error_ = error; return false; }
    return true;
}

void PoiMedia::unload() {
    SDL_DestroyTexture(texture_);
    texture_ = nullptr;
    error_.clear();
}

} // namespace gtasa
