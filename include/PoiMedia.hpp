#pragma once

#include <string>

struct SDL_Renderer;
struct SDL_Texture;

namespace gtasa {
struct PoiInfo;

class PoiMedia {
public:
    ~PoiMedia();
    bool load(SDL_Renderer* renderer, const PoiInfo& info, std::string& error);
    void unload();
    SDL_Texture* texture() const { return texture_; }
    const std::string& error() const { return error_; }

private:
    SDL_Texture* texture_{};
    std::string error_;
};
} // namespace gtasa
