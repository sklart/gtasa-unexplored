#include "Collectibles.hpp"
#include "CollectibleIcons.hpp"
#include "CollectibleInfo.hpp"
#include "CollectibleMedia.hpp"
#include "MapTexture.hpp"
#include "MapUi.hpp"
#include "MarkerSelection.hpp"
#include "Platform.hpp"
#include "PoiInfo.hpp"
#include "PoiIcon.hpp"
#include "PoiMedia.hpp"
#include "SaveParser.hpp"

#include <SDL.h>
#include <SDL_ttf.h>
#include <switch.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace {

constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;
constexpr SDL_Rect kMapRectWithPanel{0, 0, 955, 720};
constexpr SDL_Rect kMapRectFull{0, 0, kScreenW, kScreenH};
constexpr SDL_Rect kPanelRect{955, 0, 325, 720};
constexpr float kWorldHalf = 3000.0f;
constexpr float kTouchHitRadius = 48.0f;
constexpr float kControllerHitRadius = 32.0f;
constexpr float kOverlapGroupRadius = 24.0f;

struct Palette {
    SDL_Color bg{15, 18, 22, 255};
    SDL_Color mapBg{25, 31, 37, 255};
    SDL_Color grid{53, 63, 72, 255};
    SDL_Color gridMajor{76, 88, 99, 255};
    SDL_Color text{235, 239, 242, 255};
    SDL_Color muted{165, 174, 182, 255};
    SDL_Color accent{79, 172, 254, 255};
    SDL_Color warning{255, 193, 7, 255};
    SDL_Color tag{90, 200, 120, 255};
    SDL_Color snapshot{96, 165, 250, 255};
    SDL_Color horseshoe{245, 196, 72, 255};
    SDL_Color oyster{120, 220, 220, 255};
    SDL_Color jump{226, 112, 255, 255};
    SDL_Color poi{255, 121, 69, 255};
    SDL_Color selected{255, 255, 255, 255};
};

const Palette kColors{};

class TextRenderer {
public:
    bool init(SDL_Renderer* renderer) {
        renderer_ = renderer;
        if (TTF_Init() != 0) return false;
        if (R_FAILED(plInitialize(PlServiceType_User))) return false;
        plInitialized_ = true;
        if (R_FAILED(plGetSharedFontByType(&fontData_, PlSharedFontType_Standard))) return false;
        return true;
    }

    ~TextRenderer() { shutdown(); }

    void shutdown() {
        for (auto& [_, item] : cache_) SDL_DestroyTexture(item.texture);
        cache_.clear();
        for (auto& [_, font] : fonts_) TTF_CloseFont(font);
        fonts_.clear();
        if (plInitialized_) {
            plExit();
            plInitialized_ = false;
        }
        if (TTF_WasInit()) TTF_Quit();
    }

    void clearCache() {
        for (auto& [_, item] : cache_) SDL_DestroyTexture(item.texture);
        cache_.clear();
    }

    void draw(const std::string& text, int x, int y, int size, SDL_Color color,
              int maxWidth = 0, bool center = false) {
        if (text.empty()) return;
        TTF_Font* font = getFont(size);
        if (!font) return;
        const auto key = makeKey(text, size, color, maxWidth);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            SDL_Surface* surf = maxWidth > 0
                ? TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, static_cast<Uint32>(maxWidth))
                : TTF_RenderUTF8_Blended(font, text.c_str(), color);
            if (!surf) return;
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surf);
            Item item{tex, surf->w, surf->h};
            SDL_FreeSurface(surf);
            it = cache_.emplace(key, item).first;
        }
        SDL_Rect dst{x, y, it->second.w, it->second.h};
        if (center) dst.x -= dst.w / 2;
        SDL_RenderCopy(renderer_, it->second.texture, nullptr, &dst);
    }

    int width(const std::string& text, int size) {
        TTF_Font* font = getFont(size);
        if (!font) return 0;
        int w = 0, h = 0;
        TTF_SizeUTF8(font, text.c_str(), &w, &h);
        return w;
    }

private:
    struct Item { SDL_Texture* texture = nullptr; int w = 0; int h = 0; };

    TTF_Font* getFont(int size) {
        auto it = fonts_.find(size);
        if (it != fonts_.end()) return it->second;
        SDL_RWops* rw = SDL_RWFromConstMem(fontData_.address, static_cast<int>(fontData_.size));
        if (!rw) return nullptr;
        TTF_Font* font = TTF_OpenFontRW(rw, 1, size);
        if (font) fonts_[size] = font;
        return font;
    }

    static std::string makeKey(const std::string& text, int size, SDL_Color c, int wrap) {
        std::ostringstream ss;
        ss << size << ':' << wrap << ':' << static_cast<int>(c.r) << ':' << static_cast<int>(c.g)
           << ':' << static_cast<int>(c.b) << ':' << static_cast<int>(c.a) << ':' << text;
        return ss.str();
    }

    SDL_Renderer* renderer_ = nullptr;
    PlFontData fontData_{};
    bool plInitialized_ = false;
    std::map<int, TTF_Font*> fonts_;
    std::map<std::string, Item> cache_;
};

struct AppState {
    gtasa::Platform platform;
    gtasa::CollectibleIcons icons;
    gtasa::PoiIcon poiIcon;
    gtasa::CollectibleMedia media;
    gtasa::PoiMedia poiMedia;
    gtasa::AppConfig config;
    gtasa::SaveDiscovery discovery;
    gtasa::MapTexture mapTexture;
    std::vector<gtasa::SaveEntry> validSaves;
    std::vector<gtasa::ParseResult> parsed;
    int saveIndex = 0;
    bool filters[static_cast<int>(gtasa::CollectibleType::Count)]{true, true, true, true, true};
    bool legendOpen = false;
    int legendIndex = 0;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float cursorX = 0.0f;
    float cursorY = 0.0f;
    float zoom = 1.0f;
    int selected = -1;
    int selectedPoi = -1;
    bool showPanel = true;
    bool detailOpen = false;
    int detailScroll = 0;
    std::string status;
};

const SDL_Rect& mapRect(const AppState& a) {
    return a.showPanel ? kMapRectWithPanel : kMapRectFull;
}

SDL_Rect mapContentRect(const AppState& a) {
    return a.mapTexture.contentRect(gtasa::MapView{a.centerX, a.centerY, a.zoom}, mapRect(a));
}

bool isRu(const AppState& a) { return a.config.language == "ru"; }

std::string tr(const AppState& a, const char* ru, const char* en) {
    return isRu(a) ? ru : en;
}

SDL_Color typeColor(gtasa::CollectibleType t) {
    switch (t) {
        case gtasa::CollectibleType::Tag: return kColors.tag;
        case gtasa::CollectibleType::Snapshot: return kColors.snapshot;
        case gtasa::CollectibleType::Horseshoe: return kColors.horseshoe;
        case gtasa::CollectibleType::Oyster: return kColors.oyster;
        case gtasa::CollectibleType::StuntJump: return kColors.jump;
        default: return kColors.text;
    }
}

std::string typeName(const AppState& a, gtasa::CollectibleType t) {
    switch (t) {
        case gtasa::CollectibleType::Tag: return tr(a, "Граффити", "Tags");
        case gtasa::CollectibleType::Snapshot: return tr(a, "Снимки", "Snapshots");
        case gtasa::CollectibleType::Horseshoe: return tr(a, "Подковы", "Horseshoes");
        case gtasa::CollectibleType::Oyster: return tr(a, "Устрицы", "Oysters");
        case gtasa::CollectibleType::StuntJump: return tr(a, "Уник. прыжки", "Stunt jumps");
        default: return "?";
    }
}

int completedFor(const gtasa::ParseSummary& s, gtasa::CollectibleType t) {
    switch (t) {
        case gtasa::CollectibleType::Tag: return s.tagsCompleted;
        case gtasa::CollectibleType::Snapshot: return s.snapshotsCompleted;
        case gtasa::CollectibleType::Horseshoe: return s.horseshoesCompleted;
        case gtasa::CollectibleType::Oyster: return s.oystersCompleted;
        case gtasa::CollectibleType::StuntJump: return s.stuntJumpsCompleted;
        default: return 0;
    }
}
int totalFor(const gtasa::ParseSummary& s, gtasa::CollectibleType t) {
    switch (t) {
        case gtasa::CollectibleType::Tag: return s.tagsTotal;
        case gtasa::CollectibleType::Snapshot: return s.snapshotsTotal;
        case gtasa::CollectibleType::Horseshoe: return s.horseshoesTotal;
        case gtasa::CollectibleType::Oyster: return s.oystersTotal;
        case gtasa::CollectibleType::StuntJump: return s.stuntJumpsTotal;
        default: return 0;
    }
}

void fill(SDL_Renderer* r, const SDL_Rect& rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rect);
}
void line(SDL_Renderer* r, int x1, int y1, int x2, int y2, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

bool insideMap(const AppState& a, int x, int y) {
    const auto rect = mapContentRect(a);
    return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

std::pair<int, int> worldToScreen(const AppState& a, float x, float y) {
    const auto rect = mapContentRect(a);
    const float base = static_cast<float>(rect.w) / (kWorldHalf * 2.0f);
    const float scale = base * a.zoom;
    const int sx = static_cast<int>(rect.x + rect.w * 0.5f + (x - a.centerX) * scale);
    const int sy = static_cast<int>(rect.y + rect.h * 0.5f - (y - a.centerY) * scale);
    return {sx, sy};
}

std::pair<int, int> collectibleToScreen(const AppState& a, float x, float y) {
    const gtasa::MapView view{a.centerX, a.centerY, a.zoom};
    int sx = 0, sy = 0;
    if (a.mapTexture.projectWorldPoint(view, mapRect(a), x, y, sx, sy)) return {sx, sy};
    return worldToScreen(a, x, y);
}

void drawIconRing(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    int x = radius, y = 0, err = 1 - x;
    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y); SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x); SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y); SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x); SDL_RenderDrawPoint(r, cx + x, cy - y);
        ++y;
        if (err < 0) err += 2 * y + 1;
        else { --x; err += 2 * (y - x) + 1; }
    }
}

void drawCollectibleFallback(SDL_Renderer* r, int x, int y, gtasa::CollectibleType type, int size) {
    const SDL_Color c = typeColor(type);
    const int h = std::max(5, size / 2);
    switch (type) {
        case gtasa::CollectibleType::Tag:
            fill(r, SDL_Rect{x - h / 3, y - h / 3, 2 * h / 3, h}, c);
            fill(r, SDL_Rect{x - h / 4, y - h / 2, h / 2, std::max(2, h / 4)}, c); break;
        case gtasa::CollectibleType::Snapshot:
            fill(r, SDL_Rect{x - h / 2, y - h / 3, h, 2 * h / 3}, c); break;
        case gtasa::CollectibleType::Horseshoe:
            line(r, x - h / 2, y - h / 2, x - h / 2, y + h / 3, c);
            line(r, x - h / 2, y + h / 3, x, y + h / 2, c);
            line(r, x, y + h / 2, x + h / 2, y + h / 3, c);
            line(r, x + h / 2, y + h / 3, x + h / 2, y - h / 2, c); break;
        case gtasa::CollectibleType::Oyster:
            line(r, x - h / 2, y + h / 3, x + h / 2, y + h / 3, c);
            line(r, x - h / 2, y + h / 3, x, y - h / 2, c);
            line(r, x, y - h / 2, x + h / 2, y + h / 3, c); break;
        case gtasa::CollectibleType::StuntJump:
            line(r, x, y - h / 2, x + h / 2, y, c);
            line(r, x + h / 2, y, x, y + h / 2, c);
            line(r, x, y + h / 2, x - h / 2, y, c);
            line(r, x - h / 2, y, x, y - h / 2, c); break;
        default: break;
    }
}

void drawCollectibleIcon(SDL_Renderer* r, const gtasa::CollectibleIcons& icons,
                         int x, int y, gtasa::CollectibleType type, int size = 18, Uint8 alpha = 255) {
    SDL_Texture* texture = icons.texture(type);
    if (!texture) { drawCollectibleFallback(r, x, y, type, size); return; }
    int sourceW = 0, sourceH = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &sourceW, &sourceH);
    if (sourceW <= 0 || sourceH <= 0) { drawCollectibleFallback(r, x, y, type, size); return; }
    const float ratio = static_cast<float>(sourceW) / sourceH;
    const int w = std::max(1, static_cast<int>(std::lround(ratio >= 1.0f ? size : size * ratio)));
    const int h = std::max(1, static_cast<int>(std::lround(ratio >= 1.0f ? size / ratio : size)));
    SDL_Rect target{x - w / 2, y - h / 2, w, h};
    // A tiny alpha shadow separates the icon from both light terrain and dark water.
    SDL_SetTextureColorMod(texture, 8, 12, 16);
    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(std::min(220, static_cast<int>(alpha))));
    for (const auto& offset : std::array<std::pair<int, int>, 4>{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}}) {
        SDL_Rect shadow = target; shadow.x += offset.first; shadow.y += offset.second;
        SDL_RenderCopy(r, texture, nullptr, &shadow);
    }
    SDL_SetTextureColorMod(texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_RenderCopy(r, texture, nullptr, &target);
    SDL_SetTextureAlphaMod(texture, 255);
}

std::pair<int, int> collectibleIconDimensions(const gtasa::CollectibleIcons& icons,
                                               gtasa::CollectibleType type, int size) {
    SDL_Texture* texture = icons.texture(type);
    int sourceW = 0, sourceH = 0;
    if (!texture || SDL_QueryTexture(texture, nullptr, nullptr, &sourceW, &sourceH) != 0 ||
        sourceW <= 0 || sourceH <= 0) return {size, size};
    const float ratio = static_cast<float>(sourceW) / sourceH;
    const int w = std::max(1, static_cast<int>(std::lround(ratio >= 1.0f ? size : size * ratio)));
    const int h = std::max(1, static_cast<int>(std::lround(ratio >= 1.0f ? size / ratio : size)));
    return {w, h};
}

void drawPoiIcon(SDL_Renderer* r, const gtasa::PoiIcon& icon, int x, int y, int size = 18, bool selected = false) {
    SDL_Texture* texture = icon.texture();
    int sourceW = 0, sourceH = 0;
    if (!texture || SDL_QueryTexture(texture, nullptr, nullptr, &sourceW, &sourceH) != 0 ||
        sourceW <= 0 || sourceH <= 0) return;
    const int height = std::max(1, static_cast<int>(std::lround(size * static_cast<float>(sourceH) / sourceW)));
    const SDL_Rect target{x - size / 2, y - height, size, height};
    if (selected) {
        const int glowW = std::max(size + 2, static_cast<int>(std::lround(size * 1.10f)));
        const int glowH = std::max(height + 2, static_cast<int>(std::lround(height * 1.10f)));
        const SDL_Rect glow{x - glowW / 2, y - glowH, glowW, glowH};
        SDL_SetTextureColorMod(texture, 255, 245, 190);
        SDL_SetTextureAlphaMod(texture, 110);
        SDL_RenderCopy(r, texture, nullptr, &glow);
        SDL_SetTextureColorMod(texture, 255, 255, 255);
        SDL_SetTextureAlphaMod(texture, 255);
    }
    SDL_RenderCopy(r, texture, nullptr, &target);
}

void clampCamera(AppState& a) {
    a.zoom = std::max(0.85f, std::min(8.0f, a.zoom));
    a.centerX = std::max(-kWorldHalf, std::min(kWorldHalf, a.centerX));
    a.centerY = std::max(-kWorldHalf, std::min(kWorldHalf, a.centerY));
}

const gtasa::ParseResult* currentParse(const AppState& a) {
    if (a.saveIndex < 0 || a.saveIndex >= static_cast<int>(a.parsed.size())) return nullptr;
    return &a.parsed[a.saveIndex];
}
const gtasa::SaveEntry* currentSave(const AppState& a) {
    if (a.saveIndex < 0 || a.saveIndex >= static_cast<int>(a.validSaves.size())) return nullptr;
    return &a.validSaves[a.saveIndex];
}

void selectNearest(AppState& a, int px, int py, float maxDist) {
    const auto* p = currentParse(a);
    float best = maxDist * maxDist;
    int selected = -1;
    int selectedPoi = -1;
    if (p) {
        for (std::size_t i = 0; i < p->missing.size(); ++i) {
            const auto& c = p->missing[i];
            if (!a.filters[static_cast<int>(c.type)]) continue;
            const auto [sx, sy] = collectibleToScreen(a, c.x, c.y);
            if (!insideMap(a, sx, sy)) continue;
            const float dx = static_cast<float>(sx - px);
            const float dy = static_cast<float>(sy - py);
            const float d2 = dx * dx + dy * dy;
            if (d2 <= best) { best = d2; selected = static_cast<int>(i); selectedPoi = -1; }
        }
    }
    if (a.config.showPoi) {
        for (std::size_t i = 0; i < gtasa::poiInfoCount(); ++i) {
            const auto* poi = gtasa::poiInfo(i);
            if (!poi || !poi->visibleOnMap) continue;
            const auto [sx, sy] = collectibleToScreen(a, poi->x, poi->y);
            if (!insideMap(a, sx, sy)) continue;
            const float dx = static_cast<float>(sx - px), dy = static_cast<float>(sy - py);
            const float d2 = dx * dx + dy * dy;
            if (d2 <= best) { best = d2; selected = -1; selectedPoi = static_cast<int>(i); }
        }
    }
    a.selected = selected;
    a.selectedPoi = selectedPoi;
}

bool selectedMarkerScreen(const AppState& a, int& x, int& y) {
    const auto* parsed = currentParse(a);
    if (parsed && a.selected >= 0 && a.selected < static_cast<int>(parsed->missing.size())) {
        std::tie(x, y) = collectibleToScreen(a, parsed->missing[static_cast<std::size_t>(a.selected)].x,
                                              parsed->missing[static_cast<std::size_t>(a.selected)].y);
        return true;
    }
    if (const auto* poi = a.selectedPoi >= 0 ? gtasa::poiInfo(static_cast<std::size_t>(a.selectedPoi)) : nullptr) {
        std::tie(x, y) = collectibleToScreen(a, poi->x, poi->y);
        return true;
    }
    return false;
}

bool selectedMarkerHit(const AppState& a, int x, int y, float radius) {
    int sx = 0, sy = 0;
    if (!selectedMarkerScreen(a, sx, sy)) return false;
    const float dx = sx - x, dy = sy - y;
    return dx * dx + dy * dy <= radius * radius;
}

bool cycleOverlappingMarkers(AppState& a, int direction) {
    std::vector<gtasa::MarkerSelectionPoint> points;
    const auto* parsed = currentParse(a);
    if (parsed) for (std::size_t i = 0; i < parsed->missing.size(); ++i) {
        const auto& item = parsed->missing[i];
        if (!a.filters[static_cast<int>(item.type)]) continue;
        const auto [x, y] = collectibleToScreen(a, item.x, item.y);
        if (insideMap(a, x, y)) points.push_back({static_cast<int>(i) + 1, static_cast<float>(x), static_cast<float>(y)});
    }
    if (a.config.showPoi) for (std::size_t i = 0; i < gtasa::poiInfoCount(); ++i) {
        const auto* poi = gtasa::poiInfo(i);
        if (!poi || !poi->visibleOnMap) continue;
        const auto [x, y] = collectibleToScreen(a, poi->x, poi->y);
        if (insideMap(a, x, y)) points.push_back({-static_cast<int>(i) - 1, static_cast<float>(x), static_cast<float>(y)});
    }
    const int current = a.selected >= 0 ? a.selected + 1 : (a.selectedPoi >= 0 ? -a.selectedPoi - 1 : 0);
    int next = 0;
    if (!gtasa::cycleOverlappingMarker(points, current, direction, kOverlapGroupRadius, next)) return false;
    a.selected = next > 0 ? next - 1 : -1;
    a.selectedPoi = next < 0 ? -next - 1 : -1;
    if (const auto* parsedAgain = currentParse(a); parsedAgain && a.selected >= 0) {
        const auto& item = parsedAgain->missing[static_cast<std::size_t>(a.selected)];
        a.cursorX = item.x; a.cursorY = item.y;
    } else if (const auto* poi = a.selectedPoi >= 0 ? gtasa::poiInfo(static_cast<std::size_t>(a.selectedPoi)) : nullptr) {
        a.cursorX = poi->x; a.cursorY = poi->y;
    }
    return true;
}

void drawMap(SDL_Renderer* r, const AppState& a) {
    const auto& rect = mapRect(a);
    fill(r, rect, kColors.mapBg);

    const gtasa::MapView view{a.centerX, a.centerY, a.zoom};
    if (!a.mapTexture.render(r, view, rect)) {
        for (int w = -3000; w <= 3000; w += 500) {
            const auto [x1, y1] = worldToScreen(a, static_cast<float>(w), -3000.0f);
            const auto [x2, y2] = worldToScreen(a, static_cast<float>(w), 3000.0f);
            const SDL_Color c = (w % 1000 == 0) ? kColors.gridMajor : kColors.grid;
            line(r, x1, y1, x2, y2, c);
            const auto [xx1, yy1] = worldToScreen(a, -3000.0f, static_cast<float>(w));
            const auto [xx2, yy2] = worldToScreen(a, 3000.0f, static_cast<float>(w));
            line(r, xx1, yy1, xx2, yy2, c);
        }
    }

    const auto* p = currentParse(a);
    if (p) {
        for (std::size_t i = 0; i < p->missing.size(); ++i) {
            const auto& c = p->missing[i];
            if (!a.filters[static_cast<int>(c.type)]) continue;
            const auto [sx, sy] = collectibleToScreen(a, c.x, c.y);
            const int iconSize = gtasa::collectibleIconSize(a.zoom);
            const bool selected = static_cast<int>(i) == a.selected;
            const int selectedSize = selected ? static_cast<int>(std::lround(iconSize * 1.18f)) : iconSize;
            const auto [iconW, iconH] = collectibleIconDimensions(a.icons, c.type, selectedSize);
            const auto& content = mapContentRect(a);
            if (!gtasa::markerFullyVisible(sx, sy, iconW, iconH,
                                           content.x, content.y, content.w, content.h)) continue;
            const Uint8 alpha = selected ? 255 : (c.completed ? 135 : 255);
            drawCollectibleIcon(r, a.icons, sx, sy, c.type, selectedSize, alpha);
            if (selected) drawIconRing(r, sx, sy, std::max(iconW, iconH) / 2 + 2, kColors.selected);
        }
    }
    for (std::size_t i = 0; a.config.showPoi && i < gtasa::poiInfoCount(); ++i) {
        const auto* poi = gtasa::poiInfo(i);
        if (!poi || !poi->visibleOnMap) continue;
        const auto [sx, sy] = collectibleToScreen(a, poi->x, poi->y);
        const bool selected = static_cast<int>(i) == a.selectedPoi;
        const int baseIconSize = gtasa::poiMarkerSize(a.zoom);
        const int iconSize = selected ? static_cast<int>(std::lround(baseIconSize * 1.15f)) : baseIconSize;
        int sourceW = 0, sourceH = 0;
        if (SDL_Texture* texture = a.poiIcon.texture()) SDL_QueryTexture(texture, nullptr, nullptr, &sourceW, &sourceH);
        const int iconHeight = sourceW > 0 ? static_cast<int>(std::lround(iconSize * static_cast<float>(sourceH) / sourceW)) : iconSize;
        const auto& content = mapContentRect(a);
        if (!gtasa::markerFullyVisible(sx, sy, iconSize, iconHeight,
                                       content.x, content.y, content.w, content.h, true)) continue;
        drawPoiIcon(r, a.poiIcon, sx, sy, iconSize, selected);
    }

    // Controller cursor lives in world space, so it uses the exact same
    // projection as markers and remains usable at a cropped map edge.
    auto [cx, cy] = collectibleToScreen(a, a.cursorX, a.cursorY);
    line(r, cx - 8, cy, cx + 8, cy, SDL_Color{220, 220, 220, 170});
    line(r, cx, cy - 8, cx, cy + 8, SDL_Color{220, 220, 220, 170});
}

const gtasa::CollectibleInfo* selectedInfo(const AppState& a) {
    const auto* parsed = currentParse(a);
    if (!parsed || a.selected < 0 || a.selected >= static_cast<int>(parsed->missing.size())) return nullptr;
    return gtasa::collectibleInfoForRuntime(parsed->missing[static_cast<std::size_t>(a.selected)]);
}

const gtasa::PoiInfo* selectedPoiInfo(const AppState& a) {
    return a.selectedPoi >= 0 ? gtasa::poiInfo(static_cast<std::size_t>(a.selectedPoi)) : nullptr;
}

void openDetails(AppState& a, SDL_Renderer* renderer) {
    if (const auto* poi = selectedPoiInfo(a)) {
        a.detailOpen = true;
        a.detailScroll = 0;
        std::string error;
        if (!a.poiMedia.load(renderer, *poi, error))
            a.platform.log("POI image unavailable: " + std::string(poi->imagePath) + ": " + error);
        return;
    }
    const auto* info = selectedInfo(a);
    if (!info) {
        a.status = tr(a, "Подробности для этой метки недоступны", "Details unavailable for this marker");
        return;
    }
    a.detailOpen = true;
    a.detailScroll = 0;
    std::string error;
    if (!a.media.load(renderer, *info, error)) {
        a.platform.log("Collectible image unavailable: " + std::string(info->imagePath) + ": " + error);
    }
}

void centerSelectedIfNeeded(AppState& a) {
    const auto* info = selectedInfo(a);
    if (!info) return;
    const auto [x, y] = collectibleToScreen(a, info->x, info->y);
    const SDL_Rect map = mapContentRect(a);
    // Preserve the user's camera when the item is already comfortably visible.
    const SDL_Rect safe{map.x + map.w / 6, map.y + map.h / 6, map.w * 2 / 3, map.h * 2 / 3};
    if (x >= safe.x && x < safe.x + safe.w && y >= safe.y && y < safe.y + safe.h) return;
    a.centerX = info->x;
    a.centerY = info->y;
    a.cursorX = info->x;
    a.cursorY = info->y;
    clampCamera(a);
}

void selectAdjacent(AppState& a, int direction, SDL_Renderer* renderer) {
    const auto* parsed = currentParse(a);
    if (!parsed || parsed->missing.empty()) return;
    const int n = static_cast<int>(parsed->missing.size());
    const int index = gtasa::nextVisibleMarkerIndex(a.selected, n, direction, [&](int candidate) {
        return a.filters[static_cast<int>(parsed->missing[static_cast<std::size_t>(candidate)].type)];
    });
    if (index < 0) return;
    a.selected = index;
    a.selectedPoi = -1;
    centerSelectedIfNeeded(a);
    if (a.detailOpen) openDetails(a, renderer);
}

void navigateMarker(AppState& a, int direction, bool groupCycle, SDL_Renderer* renderer) {
    if (groupCycle) {
        if (cycleOverlappingMarkers(a, direction) && a.detailOpen) openDetails(a, renderer);
        return;
    }
    selectAdjacent(a, direction, renderer);
}

void drawDetails(SDL_Renderer* r, TextRenderer& text, const AppState& a) {
    if (!a.detailOpen) return;
    if (const auto* poi = selectedPoiInfo(a)) {
        fill(r, SDL_Rect{0, 0, kScreenW, kScreenH}, SDL_Color{7, 10, 13, 244});
        SDL_Rect card{66, 38, 1148, 644};
        fill(r, card, kColors.bg);
        SDL_SetRenderDrawColor(r, 92, 106, 118, 255); SDL_RenderDrawRect(r, &card);
        drawPoiIcon(r, a.poiIcon, 110, 102, 42);
        text.draw(isRu(a) ? poi->nameRu : poi->nameEn, 145, 60, 28, kColors.text, 800);
        text.draw(gtasa::poiLocationStatus(poi->representative, isRu(a)),
                  930, 66, 16, poi->representative ? kColors.warning : kColors.accent, 235);
        SDL_Rect imageRect{260, 108, 760, 350};
        if (SDL_Texture* image = a.poiMedia.texture()) SDL_RenderCopy(r, image, nullptr, &imageRect);
        else { fill(r, imageRect, SDL_Color{26, 32, 38, 255});
               text.draw(a.poiMedia.error().empty() ? tr(a, "Пакет POI не установлен", "POI pack not installed")
                                                     : tr(a, "Изображение недоступно", "Image unavailable"),
                         640, 300, 20, kColors.muted, 600, true); }
        const SDL_Rect textClip{102, 470, 1070, 134};
        SDL_RenderSetClipRect(r, &textClip);
        text.draw(isRu(a) ? poi->descriptionRu : poi->descriptionEn, 112, 474 - a.detailScroll, 19, kColors.text, 1040);
        SDL_RenderSetClipRect(r, nullptr);
        text.draw(gtasa::formatMapCoordinates(poi->x, poi->y, poi->z, !poi->representative), 112, 615, 15, kColors.muted);
        text.draw(tr(a, "B — назад    ↑/↓ — текст", "B — back    ↑/↓ — text"), 835, 646, 15, kColors.text, 360);
        return;
    }
    const auto* parsed = currentParse(a);
    const auto* info = selectedInfo(a);
    if (!parsed || !info || a.selected < 0) return;
    const auto& item = parsed->missing[static_cast<std::size_t>(a.selected)];
    fill(r, SDL_Rect{0, 0, kScreenW, kScreenH}, SDL_Color{7, 10, 13, 244});
    SDL_Rect card{66, 38, 1148, 644};
    fill(r, card, kColors.bg);
    SDL_SetRenderDrawColor(r, 92, 106, 118, 255);
    SDL_RenderDrawRect(r, &card);
    drawCollectibleIcon(r, a.icons, 110, 83, item.type, 42);
    const std::string title = typeName(a, item.type) + " #" + std::to_string(info->canonicalId);
    text.draw(title, 145, 60, 28, kColors.text);
    const std::string state = item.completed ? tr(a, "Найдено", "Completed")
        : item.found ? tr(a, "Обнаружено", "Found") : tr(a, "Не найдено", "Missing");
    text.draw(state, 1040, 66, 18, item.completed ? kColors.accent : kColors.warning, 140);

    SDL_Rect imageRect{260, 108, 760, 350};
    if (SDL_Texture* image = a.media.texture()) {
        SDL_RenderCopy(r, image, nullptr, &imageRect);
    } else {
        fill(r, imageRect, SDL_Color{26, 32, 38, 255});
        text.draw(a.media.error().empty()
                      ? tr(a, "Пакет изображений не установлен", "Collectible media pack not installed")
                      : tr(a, "Изображение недоступно", "Image unavailable"),
                  640, 300, 20, kColors.muted, 600, true);
    }
    const char* description = isRu(a) ? info->descriptionRu : info->descriptionEn;
    const SDL_Rect textClip{102, 470, 1070, 134};
    SDL_RenderSetClipRect(r, &textClip);
    text.draw(description, 112, 474 - a.detailScroll, 19, kColors.text, 1040);
    SDL_RenderSetClipRect(r, nullptr);
    text.draw(gtasa::formatMapCoordinates(info->x, info->y, info->z), 112, 615, 15, kColors.muted);
    text.draw(tr(a, "B — назад    ↑/↓ — текст    L3/R3 — объект    ZL+L3/R3 — группа", "B — back    ↑/↓ — text    L3/R3 — item    ZL+L3/R3 — group"),
              720, 646, 15, kColors.text, 440);
}

void drawPanel(SDL_Renderer* r, TextRenderer& text, const AppState& a) {
    if (!a.showPanel) return;
    fill(r, kPanelRect, kColors.bg);
    text.draw("GTASA Unexplored", 980, 22, 28, kColors.text);
    text.draw(tr(a, "San Andreas DE • Switch • read-only", "San Andreas DE • Switch • read-only"),
              980, 58, 15, kColors.muted, 285);

    const auto* p = currentParse(a);
    const auto* save = currentSave(a);
    int y = 98;
    if (!p || !save) {
        text.draw(tr(a, "Сохранение не загружено", "No save loaded"), 980, y, 20, kColors.warning, 285);
        text.draw(a.status, 980, y + 38, 16, kColors.muted, 285);
    } else {
        std::ostringstream ss;
        ss << tr(a, "Слот: ", "Slot: ") << (save->slot > 0 ? std::to_string(save->slot) : save->displayName);
        text.draw(ss.str(), 980, y, 19, kColors.text, 285);
        text.draw(save->fromBackup ? tr(a, "Локальная копия сохранения", "Local save snapshot")
                                   : tr(a, "Актуальное сохранение", "Live save"),
                  980, y + 28, 15, save->fromBackup ? kColors.warning : kColors.muted, 285);
        y += 66;

        for (int i = 0; i < static_cast<int>(gtasa::CollectibleType::Count); ++i) {
            const auto type = static_cast<gtasa::CollectibleType>(i);
            drawCollectibleIcon(r, a.icons, 987, y + 10, type, 20);
            std::ostringstream row;
            row << typeName(a, type) << ": " << completedFor(p->summary, type)
                << "/" << totalFor(p->summary, type);
            text.draw(row.str(), 1002, y, 17, a.filters[i] ? kColors.text : kColors.muted, 265);
            y += 27;
        }

        y += 8;
        if (a.selected >= 0 && a.selected < static_cast<int>(p->missing.size())) {
            const auto& c = p->missing[a.selected];
            const auto* info = gtasa::collectibleInfoForRuntime(c);
            const std::string id = info ? std::to_string(info->canonicalId) : "?";
            text.draw(typeName(a, c.type) + " #" + id, 980, y, 20, typeColor(c.type));
            text.draw(gtasa::formatMapCoordinates(c.x, c.y, c.z), 980, y + 30, 15, kColors.muted, 285);
            text.draw(info ? tr(a, "A — подробности", "A — details")
                           : tr(a, "Карточка недоступна", "Card unavailable"),
                      980, y + 50, 14, info ? kColors.accent : kColors.warning, 285);
            if (c.type == gtasa::CollectibleType::StuntJump && c.found) {
                text.draw(tr(a, "Прыжок уже обнаружен, но не выполнен", "Jump discovered, but not completed"),
                          980, y + 54, 14, kColors.warning, 285);
            }
        }
        if (const auto* poi = selectedPoiInfo(a)) {
            text.draw(isRu(a) ? poi->nameRu : poi->nameEn, 980, y, 19, kColors.poi, 285);
            text.draw(gtasa::poiLocationStatus(poi->representative, isRu(a)),
                      980, y + 28, 15, kColors.muted, 285);
            text.draw(tr(a, "A — подробности", "A — details"), 980, y + 48, 14, kColors.accent, 285);
        }
    }

    // Keep one control/action pair per line.  The panel has enough vertical
    // space and this avoids wrapping or visually joining unrelated shortcuts.
    const int cy = 425;
    text.draw(tr(a, "Управление", "Controls"), 980, cy, 18, kColors.text);
    text.draw(tr(a, "Стик / touch — карта", "Stick / touch — map"), 980, cy + 26, 14, kColors.muted, 290);
    text.draw(tr(a, "L/R / щипок — масштаб", "L/R / pinch — zoom"), 980, cy + 45, 14, kColors.muted, 290);
    text.draw(tr(a, "A — выбрать, L3/R3 — объект", "A — select, L3/R3 — item"), 980, cy + 64, 14, kColors.muted, 290);
    text.draw(tr(a, "X — фильтры, Y — карты", "X — filters, Y — maps"), 980, cy + 83, 14, kColors.muted, 290);
    text.draw(tr(a, "ZL+L3/R3 — группа, ZR+R3 / 2 пальца — панель", "ZL+L3/R3 — group, ZR+R3 / 2 fingers — panel"), 980, cy + 102, 14, kColors.muted, 290);
    text.draw(tr(a, "+ — выход", "+ — exit"), 980, cy + 121, 14, kColors.muted, 290);
    if (!a.status.empty()) text.draw(a.status, 980, 664, 13, kColors.warning, 285);
}

void drawLegend(SDL_Renderer* r, TextRenderer& text, const AppState& a) {
    if (!a.legendOpen) return;
    SDL_Rect shade{170, 100, 610, 500};
    fill(r, shade, SDL_Color{11, 14, 17, 245});
    SDL_SetRenderDrawColor(r, 95, 105, 115, 255);
    SDL_RenderDrawRect(r, &shade);
    text.draw(tr(a, "Фильтры карты", "Map filters"), 205, 130, 28, kColors.text);
    text.draw(tr(a, "↑/↓ — выбор, A — включить/выключить, X — закрыть", "↑/↓ select, A toggle, X close"),
              205, 169, 15, kColors.muted, 540);
    int y = 220;
    for (int i = 0; i <= static_cast<int>(gtasa::CollectibleType::Count); ++i) {
        SDL_Rect row{195, y - 8, 555, 44};
        if (i == a.legendIndex) fill(r, row, SDL_Color{45, 53, 61, 255});
        if (i < static_cast<int>(gtasa::CollectibleType::Count)) {
            drawCollectibleIcon(r, a.icons, 223, y + 10, static_cast<gtasa::CollectibleType>(i), 28);
            text.draw(typeName(a, static_cast<gtasa::CollectibleType>(i)), 250, y - 2, 20,
                      a.filters[i] ? kColors.text : kColors.muted);
        } else {
            drawPoiIcon(r, a.poiIcon, 223, y + 20, 22);
            text.draw("POI", 250, y - 2, 20, a.config.showPoi ? kColors.text : kColors.muted);
        }
        y += 58;
    }
    text.draw(tr(a, "POI: Проверено — точное место; Ориентировочно — район или маршрут",
                    "POI: Verified — exact place; Approximate — area or route"),
              205, 560, 13, kColors.muted, 540);
}

std::string diagnosticsText(const AppState& a) {
    std::ostringstream ss;
    ss << "GTASA Unexplored diagnostics\n";
    ss << "Target title id: 0x" << std::hex << gtasa::kGtaSaTitleId << std::dec << "\n";
    ss << "Language: " << a.config.language << "\n";
    ss << "Map id: " << a.mapTexture.currentId() << "\n";
    ss << "Collectible data schema: v1\n";
    ss << "Collectible records: " << gtasa::collectibleInfoCount() << "\n";
    ss << "Collectible media path: " << gtasa::kAppDir << "/collectibles/images\n";
    ss << "Collectible media available: " << (a.media.texture() ? "loaded" : "not loaded") << "\n";
    ss << "POI records: " << gtasa::poiInfoCount() << "\n";
    std::size_t poiMapCount = 0, poiRepresentativeCount = 0;
    for (std::size_t i = 0; i < gtasa::poiInfoCount(); ++i) {
        const auto* poi = gtasa::poiInfo(i);
        if (poi && poi->visibleOnMap) { ++poiMapCount; if (poi->representative) ++poiRepresentativeCount; }
    }
    ss << "POI map records: " << poiMapCount << " (representative: " << poiRepresentativeCount << ")\n";
    ss << "POI media path: " << gtasa::kAppDir << "/poi/images\n";
    ss << "Using backup: " << (a.discovery.usingBackup ? "yes" : "no") << "\n";
    const auto* save = currentSave(a);
    const auto* p = currentParse(a);
    if (save) ss << "Save: " << save->path << " slot=" << save->slot << "\n";
    if (const auto* info = selectedInfo(a)) {
        ss << "Selected canonical id: " << info->canonicalId << "\n";
        ss << "Selected lookup strategy: " << (info->tagSaveOrderId ? "tag_save_order_id" : "nearest_world_coordinate") << "\n";
    }
    if (p) {
        ss << "Parse: " << (p->ok ? "OK" : "FAIL") << "\n";
        ss << "PICKUPS offset: 0x" << std::hex << p->pickupsOffset << "\n";
        ss << "TAGS offset: 0x" << p->tagsOffset << "\n";
        ss << "STUNTJUMPS offset: 0x" << p->stuntJumpsOffset << std::dec << "\n";
        ss << "Tags: " << p->summary.tagsCompleted << '/' << p->summary.tagsTotal << "\n";
        ss << "Snapshots: " << p->summary.snapshotsCompleted << '/' << p->summary.snapshotsTotal << "\n";
        ss << "Horseshoes: " << p->summary.horseshoesCompleted << '/' << p->summary.horseshoesTotal << "\n";
        ss << "Oysters: " << p->summary.oystersCompleted << '/' << p->summary.oystersTotal << "\n";
        ss << "Stunt jumps: " << p->summary.stuntJumpsCompleted << '/' << p->summary.stuntJumpsTotal << "\n";
        ss << "Missing objects: " << p->missing.size() << "\n";
    } else {
        ss << "No parsed save\nError/status: " << a.status << "\n";
    }
    return ss.str();
}

void loadSaves(AppState& a, bool forceProfile) {
    gtasa::SaveParser parser;
    a.validSaves.clear();
    a.parsed.clear();
    a.selected = -1;
    a.selectedPoi = -1;
    a.discovery = a.platform.discoverSaves(a.config, forceProfile);
    if (!a.discovery.ok) {
        a.status = a.discovery.error;
        a.platform.log("Save discovery: " + a.status);
        return;
    }
#ifdef __SWITCH__
    if (gtasa::Platform::uidValid(a.discovery.uid)) {
        a.config.uid0 = a.discovery.uid.uid[0];
        a.config.uid1 = a.discovery.uid.uid[1];
    }
#endif

    for (const auto& save : a.discovery.saves) {
        auto result = parser.parseFile(save.path);
        if (result.ok) {
            a.validSaves.push_back(save);
            a.parsed.push_back(std::move(result));
        } else {
            a.platform.log("Rejected save candidate " + save.path + ": " + result.error);
        }
    }
    if (a.parsed.empty()) {
        a.status = tr(a, "Найдены файлы, но формат DE 1.112 не распознан. Проверьте автоматический log.txt.",
                         "Files found, but DE 1.112 format was not recognized. Check automatic log.txt.");
        a.platform.log("Save parsing: " + a.status);
        return;
    }

    a.saveIndex = 0;
    for (std::size_t i = 0; i < a.validSaves.size(); ++i) {
        if (a.validSaves[i].slot == a.config.preferredSlot) {
            a.saveIndex = static_cast<int>(i);
            break;
        }
    }
    if (a.validSaves[a.saveIndex].slot > 0) a.config.preferredSlot = a.validSaves[a.saveIndex].slot;
    a.platform.saveConfig(a.config);
    a.status = a.discovery.usingBackup
        ? tr(a, "Используется резервная копия", "Using backup")
        : tr(a, "Сохранение загружено", "Save loaded");
}

void switchSlot(AppState& a, int delta) {
    if (a.parsed.empty()) return;
    const int n = static_cast<int>(a.parsed.size());
    a.saveIndex = (a.saveIndex + delta + n) % n;
    a.selected = -1;
    a.selectedPoi = -1;
    if (a.validSaves[a.saveIndex].slot > 0) {
        a.config.preferredSlot = a.validSaves[a.saveIndex].slot;
        a.platform.saveConfig(a.config);
    }
}

} // namespace

int main(int, char**) {
    AppState app;
    std::string platformError;
    if (!app.platform.initialize(platformError)) return 2;
    app.config = app.platform.loadConfig();
    loadSaves(app, false);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) return 3;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Window* window = SDL_CreateWindow("GTASA Unexplored", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          kScreenW, kScreenH, SDL_WINDOW_FULLSCREEN);
    if (!window) return 4;
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return 5;

    if (!app.icons.load(renderer)) app.platform.log("Embedded collectible icons unavailable; using procedural fallback");
    if (!app.poiIcon.load(renderer)) app.platform.log("Embedded POI icon unavailable; POI markers disabled");

    TextRenderer text;
    if (!text.init(renderer)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 6;
    }
    {
        std::string mapStatus;
        if (!app.mapTexture.discoverAndLoad(renderer, "", mapStatus)) {
            app.mapTexture.loadFallback(renderer, mapStatus);
        }
        if (app.status.empty() && !mapStatus.empty()) app.status = mapStatus;
    }
    app.platform.log(diagnosticsText(app));

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    bool running = true;
    std::map<SDL_FingerID, SDL_FPoint> touches;
    std::map<SDL_FingerID, SDL_FPoint> touchStarts;
    bool touchMoved = false;
    bool multiTouch = false;
    bool twoFingerTapCandidate = false;
    float pinchDistance = 0.0f;
    Uint32 lastTapTime = 0;
    int lastTapX = 0, lastTapY = 0;
    while (running && appletMainLoop()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_FINGERDOWN && !app.legendOpen && !app.detailOpen) {
                const int x = static_cast<int>(event.tfinger.x * kScreenW);
                const int y = static_cast<int>(event.tfinger.y * kScreenH);
                if (insideMap(app, x, y)) {
                    const SDL_FPoint point{static_cast<float>(x), static_cast<float>(y)};
                    touches[event.tfinger.fingerId] = point;
                    touchStarts[event.tfinger.fingerId] = point;
                    if (touches.size() == 1) {
                        touchMoved = false;
                        multiTouch = false;
                        twoFingerTapCandidate = false;
                    } else if (touches.size() == 2) {
                        auto first = touches.begin();
                        auto second = std::next(first);
                        const float dx = second->second.x - first->second.x;
                        const float dy = second->second.y - first->second.y;
                        pinchDistance = std::sqrt(dx * dx + dy * dy);
                        multiTouch = true;
                        // A second finger added after a drag must never turn
                        // that gesture into a two-finger panel tap.
                        twoFingerTapCandidate = !touchMoved;
                    }
                }
            }
            if (event.type == SDL_FINGERMOTION && !app.legendOpen && !app.detailOpen) {
                const auto current = touches.find(event.tfinger.fingerId);
                if (current == touches.end()) continue;
                const SDL_FPoint previous = current->second;
                const SDL_FPoint point{event.tfinger.x * kScreenW, event.tfinger.y * kScreenH};
                current->second = point;
                if (touches.size() == 1) {
                    const auto start = touchStarts.find(event.tfinger.fingerId);
                    if (start == touchStarts.end()) continue;
                    const bool wasDragging = touchMoved;
                    if (!touchMoved && !gtasa::exceedsTouchDragThreshold(start->second.x, start->second.y,
                                                                          point.x, point.y)) continue;
                    const float dx = wasDragging ? point.x - previous.x : point.x - start->second.x;
                    const float dy = wasDragging ? point.y - previous.y : point.y - start->second.y;
                    if (dx != 0.0f || dy != 0.0f) {
                        gtasa::MapView view{app.centerX, app.centerY, app.zoom};
                        app.mapTexture.panByScreenDelta(view, mapRect(app), dx, dy);
                        app.centerX = view.centerX;
                        app.centerY = view.centerY;
                        clampCamera(app);
                        touchMoved = true;
                        // A drag cannot be the first half of a later double tap.
                        lastTapTime = 0;
                    }
                } else if (touches.size() == 2) {
                    auto first = touches.begin();
                    auto second = std::next(first);
                    const float dx = second->second.x - first->second.x;
                    const float dy = second->second.y - first->second.y;
                    const float distance = std::sqrt(dx * dx + dy * dy);
                    const auto firstStart = touchStarts.find(first->first);
                    const auto secondStart = touchStarts.find(second->first);
                    if (firstStart != touchStarts.end() && secondStart != touchStarts.end() &&
                        (gtasa::exceedsTouchDragThreshold(firstStart->second.x, firstStart->second.y,
                                                           first->second.x, first->second.y) ||
                         gtasa::exceedsTouchDragThreshold(secondStart->second.x, secondStart->second.y,
                                                           second->second.x, second->second.y))) {
                        twoFingerTapCandidate = false;
                    }
                    if (pinchDistance > 1.0f && std::abs(distance - pinchDistance) >= 2.0f) {
                        app.zoom *= distance / pinchDistance;
                        clampCamera(app);
                        twoFingerTapCandidate = false;
                        touchMoved = true;
                        lastTapTime = 0;
                    }
                    pinchDistance = distance;
                }
            }
            if (event.type == SDL_FINGERUP) {
                const auto current = touches.find(event.tfinger.fingerId);
                if (current == touches.end()) continue;
                const int x = static_cast<int>(event.tfinger.x * kScreenW);
                const int y = static_cast<int>(event.tfinger.y * kScreenH);
                const bool singleTap = touches.size() == 1 && gtasa::isTapGesture(touchMoved, multiTouch);
                touches.erase(current);
                touchStarts.erase(event.tfinger.fingerId);
                if (singleTap && !app.legendOpen && !app.detailOpen && insideMap(app, x, y)) {
                    const Uint32 now = SDL_GetTicks();
                    const int dx = x - lastTapX, dy = y - lastTapY;
                    const int previous = app.selected;
                    const int previousPoi = app.selectedPoi;
                    const bool selectedHit = selectedMarkerHit(app, x, y, kTouchHitRadius);
                    if (!selectedHit) selectNearest(app, x, y, kTouchHitRadius);
                    const bool hitMarker = app.selected >= 0 || app.selectedPoi >= 0;
                    if (selectedHit || (hitMarker && (app.selected == previous || app.selectedPoi == previousPoi))) {
                        // A repeated touch on an already selected marker always
                        // opens its card, including when nearby markers overlap.
                        openDetails(app, renderer);
                        lastTapTime = 0;
                    } else if (gtasa::canResetMapFromDoubleTap(singleTap, hitMarker) &&
                               now - lastTapTime <= 350 && dx * dx + dy * dy <= 900) {
                        // Reserve double-tap reset for genuinely empty map space.
                        app.centerX = 0.0f; app.centerY = 0.0f; app.cursorX = 0.0f; app.cursorY = 0.0f; app.zoom = 1.0f;
                        app.selected = -1; app.selectedPoi = -1; lastTapTime = 0;
                    } else {
                        lastTapTime = now;
                        lastTapX = x;
                        lastTapY = y;
                    }
                }
                if (multiTouch && touches.empty()) {
                    if (twoFingerTapCandidate) app.showPanel = !app.showPanel;
                    multiTouch = false;
                    twoFingerTapCandidate = false;
                }
            }
        }

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        const u64 held = padGetButtons(&pad);
        const HidAnalogStickState left = padGetStickPos(&pad, 0);

        if (down & HidNpadButton_Plus) running = false;
        const bool groupModifier = (held & HidNpadButton_ZL) != 0;
        if (app.detailOpen) {
            if (down & HidNpadButton_B) { app.detailOpen = false; app.media.unload(); app.poiMedia.unload(); }
            if (down & HidNpadButton_Up) app.detailScroll = std::max(0, app.detailScroll - 20);
            if (down & HidNpadButton_Down) app.detailScroll = std::min(120, app.detailScroll + 20);
            if (down & HidNpadButton_StickL)
                navigateMarker(app, -1, gtasa::shouldCycleOverlap(false, groupModifier, true), renderer);
            if (down & HidNpadButton_StickR)
                navigateMarker(app, 1, gtasa::shouldCycleOverlap(false, groupModifier, true), renderer);
        } else if (app.legendOpen) {
            if (down & HidNpadButton_X) app.legendOpen = false;
            if (gtasa::shouldToggleLanguage(true, (down & HidNpadButton_ZL) != 0)) {
                app.config.language = isRu(app) ? "en" : "ru";
                app.platform.saveConfig(app.config);
                text.clearCache();
                app.status = tr(app, "Язык: Русский", "Language: English");
            }
            constexpr int kFilterCount = static_cast<int>(gtasa::CollectibleType::Count) + 1;
            if (down & HidNpadButton_Up) app.legendIndex = (app.legendIndex + kFilterCount - 1) % kFilterCount;
            if (down & HidNpadButton_Down) app.legendIndex = (app.legendIndex + 1) % kFilterCount;
            if (down & HidNpadButton_A) {
                if (app.legendIndex < static_cast<int>(gtasa::CollectibleType::Count))
                    app.filters[app.legendIndex] = !app.filters[app.legendIndex];
                else {
                    app.config.showPoi = !app.config.showPoi;
                    app.platform.saveConfig(app.config);
                    if (!app.config.showPoi) app.selectedPoi = -1;
                }
            }
        } else {
            if (down & HidNpadButton_X) app.legendOpen = true;
            if (down & HidNpadButton_Minus) loadSaves(app, true);
            if (down & HidNpadButton_Up) app.mapTexture.cycle(renderer, -1, app.status);
            if (down & HidNpadButton_Down) app.mapTexture.cycle(renderer, 1, app.status);
            if (down & HidNpadButton_Left) switchSlot(app, -1);
            if (down & HidNpadButton_Right) switchSlot(app, 1);
            if (down & HidNpadButton_StickL)
                navigateMarker(app, -1, gtasa::shouldCycleOverlap(false, groupModifier, true), renderer);
            if (down & HidNpadButton_StickR) {
                if ((held & HidNpadButton_ZR) != 0) app.showPanel = !app.showPanel;
                else navigateMarker(app, 1, gtasa::shouldCycleOverlap(false, groupModifier, true), renderer);
            }
            if (down & HidNpadButton_Y) {
                app.centerX = 0.0f; app.centerY = 0.0f; app.cursorX = 0.0f; app.cursorY = 0.0f; app.zoom = 1.0f; app.selected = -1; app.selectedPoi = -1;
                if (!app.mapTexture.discoverAndLoad(renderer, app.mapTexture.currentId(), app.status)) {
                    app.mapTexture.loadFallback(renderer, app.status);
                }
            }
            if (down & HidNpadButton_A) {
                const auto [cursorX, cursorY] = collectibleToScreen(app, app.cursorX, app.cursorY);
                const int previous = app.selected;
                const int previousPoi = app.selectedPoi;
                const bool selectedHit = selectedMarkerHit(app, cursorX, cursorY, kControllerHitRadius);
                if (!selectedHit) selectNearest(app, cursorX, cursorY, kControllerHitRadius);
                if (selectedHit || (app.selected >= 0 && app.selected == previous) ||
                    (app.selectedPoi >= 0 && app.selectedPoi == previousPoi)) openDetails(app, renderer);
            }
            if (down & HidNpadButton_B) { app.selected = -1; app.selectedPoi = -1; }

            const float cursorSpeed = 16.0f / app.zoom;
            if (std::abs(left.x) > 2500) app.cursorX += (static_cast<float>(left.x) / 32768.0f) * cursorSpeed;
            if (std::abs(left.y) > 2500) app.cursorY += (static_cast<float>(left.y) / 32768.0f) * cursorSpeed;
            app.cursorX = std::clamp(app.cursorX, -3000.0f, 3000.0f);
            app.cursorY = std::clamp(app.cursorY, -3000.0f, 3000.0f);
            // Following the cursor makes every edge marker reachable without
            // relying on an invisible inner cursor rectangle.
            app.centerX = app.cursorX;
            app.centerY = app.cursorY;
            if (held & HidNpadButton_L) app.zoom *= 0.985f;
            if (held & HidNpadButton_R) app.zoom *= 1.015f;
            clampCamera(app);
        }

        SDL_SetRenderDrawColor(renderer, kColors.bg.r, kColors.bg.g, kColors.bg.b, 255);
        SDL_RenderClear(renderer);
        drawMap(renderer, app);
        drawPanel(renderer, text, app);
        drawLegend(renderer, text, app);
        drawDetails(renderer, text, app);
        SDL_RenderPresent(renderer);
    }

    text.shutdown();
    app.media.unload();
    app.poiMedia.unload();
    app.poiIcon.unload();
    app.icons.unload();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    app.platform.shutdown();
    return 0;
}
