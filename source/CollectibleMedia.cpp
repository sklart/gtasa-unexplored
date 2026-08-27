#include "CollectibleMedia.hpp"

#include "CollectibleInfo.hpp"
#include "Platform.hpp"

#include <SDL.h>
#include <SDL_image.h>

namespace gtasa {

CollectibleMedia::~CollectibleMedia() { unload(); }

bool CollectibleMedia::load(SDL_Renderer* renderer, const CollectibleInfo& info, std::string& error) {
    unload();
    error_.clear();
    path_ = std::string(kAppDir) + "/collectibles/" + info.imagePath;
    SDL_Surface* surface = IMG_Load(path_.c_str());
    if (!surface) { error = IMG_GetError(); error_ = error; return false; }
    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture_) { error = SDL_GetError(); error_ = error; return false; }
    return true;
}

void CollectibleMedia::unload() {
    SDL_DestroyTexture(texture_);
    texture_ = nullptr;
    path_.clear();
    error_.clear();
}

} // namespace gtasa
