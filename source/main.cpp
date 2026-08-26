#include "Collectibles.hpp"
#include "MapTexture.hpp"
#include "Platform.hpp"
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
constexpr SDL_Rect kMapRect{0, 0, 955, 720};
constexpr SDL_Rect kPanelRect{955, 0, 325, 720};
constexpr float kWorldHalf = 3000.0f;

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
    float zoom = 1.0f;
    int selected = -1;
    std::string status;
};

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

bool insideMap(int x, int y) {
    return x >= kMapRect.x && x < kMapRect.x + kMapRect.w &&
           y >= kMapRect.y && y < kMapRect.y + kMapRect.h;
}

std::pair<int, int> worldToScreen(const AppState& a, float x, float y) {
    const float base = static_cast<float>(kMapRect.w) / (kWorldHalf * 2.0f);
    const float scale = base * a.zoom;
    const int sx = static_cast<int>(kMapRect.x + kMapRect.w * 0.5f + (x - a.centerX) * scale);
    const int sy = static_cast<int>(kMapRect.y + kMapRect.h * 0.5f - (y - a.centerY) * scale);
    return {sx, sy};
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
    if (!p) return;
    float best = maxDist * maxDist;
    int bestIndex = -1;
    for (std::size_t i = 0; i < p->missing.size(); ++i) {
        const auto& c = p->missing[i];
        if (!a.filters[static_cast<int>(c.type)]) continue;
        const auto [sx, sy] = worldToScreen(a, c.x, c.y);
        if (!insideMap(sx, sy)) continue;
        const float dx = static_cast<float>(sx - px);
        const float dy = static_cast<float>(sy - py);
        const float d2 = dx * dx + dy * dy;
        if (d2 < best) { best = d2; bestIndex = static_cast<int>(i); }
    }
    a.selected = bestIndex;
}

void drawCityHint(SDL_Renderer*, TextRenderer& t, const AppState& a,
                  const std::string& name, float x, float y) {
    const auto [sx, sy] = worldToScreen(a, x, y);
    if (!insideMap(sx, sy)) return;
    t.draw(name, sx, sy, 18, SDL_Color{105, 116, 126, 255}, 0, true);
}

void drawMap(SDL_Renderer* r, TextRenderer& text, const AppState& a) {
    fill(r, kMapRect, kColors.mapBg);

    const gtasa::MapView view{a.centerX, a.centerY, a.zoom};
    if (!a.mapTexture.render(r, view, kMapRect)) {
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

    drawCityHint(r, text, a, tr(a, "ЛОС-САНТОС", "LOS SANTOS"), 1750.0f, -1750.0f);
    drawCityHint(r, text, a, tr(a, "САН-ФИЕРРО", "SAN FIERRO"), -1900.0f, 550.0f);
    drawCityHint(r, text, a, tr(a, "ЛАС-ВЕНТУРАС", "LAS VENTURAS"), 1700.0f, 1500.0f);

    const auto* p = currentParse(a);
    if (p) {
        for (std::size_t i = 0; i < p->missing.size(); ++i) {
            const auto& c = p->missing[i];
            if (!a.filters[static_cast<int>(c.type)]) continue;
            const auto [sx, sy] = worldToScreen(a, c.x, c.y);
            if (!insideMap(sx, sy)) continue;
            SDL_Color col = typeColor(c.type);
            const int rad = (static_cast<int>(i) == a.selected) ? 7 : 4;
            SDL_Rect marker{sx - rad, sy - rad, rad * 2 + 1, rad * 2 + 1};
            fill(r, marker, col);
            if (static_cast<int>(i) == a.selected) {
                SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                SDL_Rect outline{sx - rad - 2, sy - rad - 2, rad * 2 + 5, rad * 2 + 5};
                SDL_RenderDrawRect(r, &outline);
            }
        }
    }

    // Crosshair for controller selection.
    const int cx = kMapRect.x + kMapRect.w / 2;
    const int cy = kMapRect.y + kMapRect.h / 2;
    line(r, cx - 8, cy, cx + 8, cy, SDL_Color{220, 220, 220, 170});
    line(r, cx, cy - 8, cx, cy + 8, SDL_Color{220, 220, 220, 170});
}

void drawPanel(SDL_Renderer* r, TextRenderer& text, const AppState& a) {
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
        text.draw(save->fromBackup ? tr(a, "Резервная копия (игра запущена)", "Backup (game is running)")
                                   : tr(a, "Актуальное сохранение", "Live save"),
                  980, y + 28, 15, save->fromBackup ? kColors.warning : kColors.muted, 285);
        y += 66;

        for (int i = 0; i < static_cast<int>(gtasa::CollectibleType::Count); ++i) {
            const auto type = static_cast<gtasa::CollectibleType>(i);
            SDL_Rect box{980, y + 5, 10, 10};
            fill(r, box, typeColor(type));
            std::ostringstream row;
            row << typeName(a, type) << ": " << completedFor(p->summary, type)
                << "/" << totalFor(p->summary, type);
            text.draw(row.str(), 998, y, 17, a.filters[i] ? kColors.text : kColors.muted, 270);
            y += 27;
        }

        y += 8;
        if (a.selected >= 0 && a.selected < static_cast<int>(p->missing.size())) {
            const auto& c = p->missing[a.selected];
            text.draw(typeName(a, c.type) + " #" + std::to_string(c.id), 980, y, 20, typeColor(c.type));
            std::ostringstream pos;
            pos.setf(std::ios::fixed); pos.precision(1);
            pos << "X " << c.x << "   Y " << c.y << "   Z " << c.z;
            text.draw(pos.str(), 980, y + 30, 15, kColors.muted, 285);
            if (c.type == gtasa::CollectibleType::StuntJump && c.found) {
                text.draw(tr(a, "Прыжок уже обнаружен, но не выполнен", "Jump discovered, but not completed"),
                          980, y + 54, 14, kColors.warning, 285);
            }
        }
    }

    const int cy = 548;
    text.draw(tr(a, "Управление", "Controls"), 980, cy, 18, kColors.text);
    text.draw(tr(a, "L/R — масштаб   левый стик — карта", "L/R — zoom   Left stick — pan"), 980, cy + 28, 14, kColors.muted, 290);
    text.draw(tr(a, "↑/↓ — подложка   Y — перечитать", "↑/↓ — map layer   Y — reload"), 980, cy + 50, 14, kColors.muted, 290);
    text.draw(tr(a, "←/→ — слот   − — профиль   ZL — язык", "←/→ — slot   − — profile   ZL — language"), 980, cy + 72, 14, kColors.muted, 290);
    text.draw(tr(a, "ZR — диагностика   + — выход", "ZR — diagnostics   + — exit"), 980, cy + 94, 14, kColors.muted, 290);
    if (!a.status.empty()) text.draw(a.status, 980, 674, 13, kColors.warning, 285);
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
    for (int i = 0; i < static_cast<int>(gtasa::CollectibleType::Count); ++i) {
        SDL_Rect row{195, y - 8, 555, 44};
        if (i == a.legendIndex) fill(r, row, SDL_Color{45, 53, 61, 255});
        SDL_Rect swatch{215, y + 2, 16, 16};
        fill(r, swatch, typeColor(static_cast<gtasa::CollectibleType>(i)));
        text.draw(a.filters[i] ? "✓" : "—", 246, y - 2, 20, a.filters[i] ? kColors.text : kColors.muted);
        text.draw(typeName(a, static_cast<gtasa::CollectibleType>(i)), 278, y - 2, 20, kColors.text);
        y += 58;
    }
}

std::string diagnosticsText(const AppState& a) {
    std::ostringstream ss;
    ss << "GTASA Unexplored diagnostics\n";
    ss << "Target title id: 0x" << std::hex << gtasa::kGtaSaTitleId << std::dec << "\n";
    ss << "Language: " << a.config.language << "\n";
    ss << "Map id: " << a.mapTexture.currentId() << "\n";
    ss << "Using backup: " << (a.discovery.usingBackup ? "yes" : "no") << "\n";
    const auto* save = currentSave(a);
    const auto* p = currentParse(a);
    if (save) ss << "Save: " << save->path << " slot=" << save->slot << "\n";
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
    a.discovery = a.platform.discoverSaves(a.config, forceProfile);
    if (!a.discovery.ok) {
        a.status = a.discovery.error;
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
        a.status = tr(a, "Найдены файлы, но формат DE 1.112 не распознан. Экспортируйте диагностику ZR.",
                         "Files found, but DE 1.112 format was not recognized. Export diagnostics with ZR.");
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
        if (!mapStatus.empty()) app.status = mapStatus;
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    bool running = true;
    while (running && appletMainLoop()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_FINGERDOWN && !app.legendOpen) {
                const int x = static_cast<int>(event.tfinger.x * kScreenW);
                const int y = static_cast<int>(event.tfinger.y * kScreenH);
                if (insideMap(x, y)) selectNearest(app, x, y, 34.0f);
            }
        }

        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        const u64 held = padGetButtons(&pad);
        const HidAnalogStickState left = padGetStickPos(&pad, 0);

        if (down & HidNpadButton_Plus) running = false;
        if (down & HidNpadButton_ZL) {
            app.config.language = isRu(app) ? "en" : "ru";
            app.platform.saveConfig(app.config);
            text.clearCache();
            app.status = tr(app, "Язык: Русский", "Language: English");
        }

        if (app.legendOpen) {
            if (down & HidNpadButton_X) app.legendOpen = false;
            if (down & HidNpadButton_Up) app.legendIndex = (app.legendIndex + 4) % 5;
            if (down & HidNpadButton_Down) app.legendIndex = (app.legendIndex + 1) % 5;
            if (down & HidNpadButton_A) app.filters[app.legendIndex] = !app.filters[app.legendIndex];
        } else {
            if (down & HidNpadButton_X) app.legendOpen = true;
            if (down & HidNpadButton_Minus) loadSaves(app, true);
            if (down & HidNpadButton_Up) app.mapTexture.cycle(renderer, -1, app.status);
            if (down & HidNpadButton_Down) app.mapTexture.cycle(renderer, 1, app.status);
            if (down & HidNpadButton_Left) switchSlot(app, -1);
            if (down & HidNpadButton_Right) switchSlot(app, 1);
            if (down & HidNpadButton_Y) {
                app.centerX = 0.0f; app.centerY = 0.0f; app.zoom = 1.0f; app.selected = -1;
                if (!app.mapTexture.discoverAndLoad(renderer, app.mapTexture.currentId(), app.status)) {
                    app.mapTexture.loadFallback(renderer, app.status);
                }
            }
            if (down & HidNpadButton_A) {
                selectNearest(app, kMapRect.x + kMapRect.w / 2, kMapRect.y + kMapRect.h / 2, 48.0f);
            }
            if (down & HidNpadButton_B) app.selected = -1;
            if (down & HidNpadButton_ZR) {
                const bool ok = app.platform.exportDiagnostics(diagnosticsText(app));
                app.status = ok
                    ? tr(app, "diagnostics.txt сохранён на SD", "diagnostics.txt saved to SD")
                    : tr(app, "Не удалось сохранить диагностику", "Failed to save diagnostics");
            }

            const float panSpeed = 16.0f / app.zoom;
            if (std::abs(left.x) > 2500) app.centerX += (static_cast<float>(left.x) / 32768.0f) * panSpeed;
            if (std::abs(left.y) > 2500) app.centerY += (static_cast<float>(left.y) / 32768.0f) * panSpeed;
            if (held & HidNpadButton_L) app.zoom *= 0.985f;
            if (held & HidNpadButton_R) app.zoom *= 1.015f;
            clampCamera(app);
        }

        SDL_SetRenderDrawColor(renderer, kColors.bg.r, kColors.bg.g, kColors.bg.b, 255);
        SDL_RenderClear(renderer);
        drawMap(renderer, text, app);
        drawPanel(renderer, text, app);
        drawLegend(renderer, text, app);
        SDL_RenderPresent(renderer);
    }

    text.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    app.platform.shutdown();
    return 0;
}
