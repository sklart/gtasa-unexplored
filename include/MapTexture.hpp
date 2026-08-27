#pragma once
#include <algorithm>
#include <string>
#include <vector>
struct SDL_Renderer; struct SDL_Texture; struct SDL_Rect;
namespace gtasa {
struct MapView { float centerX{}; float centerY{}; float zoom{1.0f}; };
inline void clampMapView(MapView& v, float maxZoom) { v.zoom = std::clamp(v.zoom, 0.85f, maxZoom); v.centerX = std::clamp(v.centerX, -3000.0f, 3000.0f); v.centerY = std::clamp(v.centerY, -3000.0f, 3000.0f); }
inline float worldToMapPixelX(float x, float size) { return (x + 3000.0f) * size / 6000.0f; }
inline float worldToMapPixelY(float y, float size) { return (3000.0f - y) * size / 6000.0f; }
struct MapEntry {
    std::string id, nameRu, nameEn, descriptionRu, descriptionEn, credit, path;
    // Per-layer world calibration.  Defaults preserve the v1 full-world map.
    float left{-3000.0f}, right{3000.0f}, top{3000.0f}, bottom{-3000.0f};
};
struct MapPack { int format{}; int canvasSize{}; std::string projection, name; };
class MapTexture {
public:
    ~MapTexture();
    const std::string& currentId() const;
    std::string currentName(bool russian) const;
    std::string currentDescription(bool russian) const;
    std::string currentCredit() const;
    MapEntry activeCalibration() const;
    void unload();
    bool discover(std::string& status);
    bool discoverAndLoad(SDL_Renderer* renderer, const std::string& preferredId, std::string& status);
    bool loadFallback(SDL_Renderer* renderer, std::string& status);
    bool cycle(SDL_Renderer* renderer, int delta, std::string& status);
    // The actual, aspect-correct area occupied by the map in a UI viewport.
    SDL_Rect contentRect(const SDL_Rect& viewport) const;
    bool render(SDL_Renderer* renderer, const MapView& view, const SDL_Rect& dst) const;
    // Projects with precisely the same cropped source rectangle as render().
    bool projectWorldPoint(const MapView& view, const SDL_Rect& dst, float x, float y,
                           int& screenX, int& screenY) const;
    bool screenToWorld(const MapView& view, const SDL_Rect& dst, int screenX, int screenY,
                       float& worldX, float& worldY) const;
    void panByScreenDelta(MapView& view, const SDL_Rect& dst, float dx, float dy) const;

private:
    bool parseManifest(const std::string& path, std::string& error);
    bool tryLoad(SDL_Renderer* renderer, const std::string& path, std::string& error);
    bool loadIndex(SDL_Renderer* renderer, int index, std::string& status);
    std::vector<MapEntry> maps_;
    MapPack pack_;
    int index_{-1};
    SDL_Texture* texture_{};
    int width_{};
    int height_{};
    bool fallback_{};
    std::string source_;
};
}
