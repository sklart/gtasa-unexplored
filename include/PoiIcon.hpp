#pragma once

struct SDL_Renderer;
struct SDL_Texture;

namespace gtasa {

class PoiIcon {
public:
    ~PoiIcon();
    bool load(SDL_Renderer* renderer);
    void unload();
    SDL_Texture* texture() const { return texture_; }

private:
    SDL_Texture* texture_ = nullptr;
};

} // namespace gtasa
