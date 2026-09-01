#include "MapTexture.hpp"
#include "MapProjection.hpp"
#include "Platform.hpp"
#include "fallback_map_bin.h"
#include <SDL_image.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <cerrno>
#include <climits>
#include <cstdlib>

namespace gtasa {
namespace {

constexpr int kMapPackFormat = 1;
constexpr const char* kProjection = "sa-world-v1";

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string stem(const std::string& name) {
    const auto slash = name.find_last_of("/\\");
    const auto start = slash == std::string::npos ? 0 : slash + 1;
    const auto dot = name.find_last_of('.');
    return name.substr(start, dot == std::string::npos || dot < start ? std::string::npos : dot - start);
}

std::string prettyName(std::string s) {
    for (char& c : s) if (c == '_' || c == '-') c = ' ';
    bool upperNext = true;
    for (char& c : s) {
        if (c == ' ') upperNext = true;
        else if (upperNext) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); upperNext = false; }
    }
    return s;
}

std::string joinPath(const std::string& dir, const std::string& file) {
    if (file.empty()) return dir;
    if (file.find(":/") != std::string::npos || (!file.empty() && file[0] == '/')) return file;
    return dir + (dir.empty() || dir.back() == '/' ? "" : "/") + file;
}

bool parseIntStrict(const std::string& value, int& out) {
    if (value.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) return false;
    out = static_cast<int>(parsed);
    return true;
}

bool parseFloatStrict(const std::string& value, float& out) {
    if (value.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const float parsed = std::strtof(value.c_str(), &end);
    if (errno != 0 || end == value.c_str() || *end != '\0' || !std::isfinite(parsed)) return false;
    out = parsed;
    return true;
}

} // namespace

MapTexture::~MapTexture() { unload(); }

const std::string& MapTexture::currentId() const {
    static const std::string fallbackId = "built-in-open-svg";
    static const std::string empty;
    if (fallback_) return fallbackId;
    return index_ >= 0 && index_ < static_cast<int>(maps_.size()) ? maps_[static_cast<std::size_t>(index_)].id : empty;
}

std::string MapTexture::currentName(bool russian) const {
    if (index_ < 0 || index_ >= static_cast<int>(maps_.size())) return {};
    const auto& m = maps_[static_cast<std::size_t>(index_)];
    if (russian && !m.nameRu.empty()) return m.nameRu;
    if (!m.nameEn.empty()) return m.nameEn;
    if (!m.nameRu.empty()) return m.nameRu;
    return m.id;
}

std::string MapTexture::currentDescription(bool russian) const {
    if (index_ < 0 || index_ >= static_cast<int>(maps_.size())) return {};
    const auto& m = maps_[static_cast<std::size_t>(index_)];
    if (russian && !m.descriptionRu.empty()) return m.descriptionRu;
    if (!m.descriptionEn.empty()) return m.descriptionEn;
    if (!m.descriptionRu.empty()) return m.descriptionRu;
    return {};
}

std::string MapTexture::currentCredit() const {
    if (index_ < 0 || index_ >= static_cast<int>(maps_.size())) return {};
    return maps_[static_cast<std::size_t>(index_)].credit;
}

MapEntry MapTexture::activeCalibration() const {
    return index_ >= 0 && index_ < static_cast<int>(maps_.size()) ? maps_[static_cast<std::size_t>(index_)] : MapEntry{};
}

void MapTexture::unload() {
    if (texture_) SDL_DestroyTexture(texture_);
    texture_ = nullptr;
    width_ = 0;
    height_ = 0;
    fallback_ = false;
    source_.clear();
}

bool MapTexture::parseManifest(const std::string& path, std::string& error) {
    std::ifstream f(path);
    if (!f) { error = "maps.ini not found"; return false; }

    const auto slash = path.find_last_of("/\\");
    const std::string dir = slash == std::string::npos ? std::string{} : path.substr(0, slash);
    maps_.clear();
    pack_ = {};
    MapEntry current;
    enum class Section { None, Pack, Map, Other } section = Section::None;
    bool mapUsable = false;
    std::set<std::string> ids;
    int legacyCanvasSize = 0;
    bool hasCanvasSize = false;

    auto resetMap = [&]() { current = {}; mapUsable = false; };
    auto commitMap = [&]() {
        if (section != Section::Map) return;
        if (!mapUsable) { resetMap(); return; }
        if (current.id.empty() && !current.path.empty()) current.id = stem(current.path);
        if (current.nameEn.empty()) current.nameEn = prettyName(current.id);
        if (current.nameRu.empty()) current.nameRu = current.nameEn;
        if (current.id.empty() || current.path.empty()) { resetMap(); return; }
        if (!ids.insert(current.id).second) { error = "Duplicate map id in maps.ini: " + current.id; resetMap(); return; }
        maps_.push_back(current);
        resetMap();
    };

    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            commitMap();
            const auto name = lower(trim(line.substr(1, line.size() - 2)));
            if (name == "pack") section = Section::Pack;
            else if (name == "map") { section = Section::Map; mapUsable = true; }
            else section = Section::Other;
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto key = lower(trim(line.substr(0, eq)));
        const auto value = trim(line.substr(eq + 1));
        if (section == Section::Pack) {
            if (key == "format" && !parseIntStrict(value, pack_.format)) error = "Invalid format value in [pack]";
            else if (key == "canvas_size") {
                hasCanvasSize = parseIntStrict(value, pack_.canvasSize);
                if (!hasCanvasSize) error = "Invalid canvas_size value in [pack]";
            } else if (key == "canvas" && !parseIntStrict(value, legacyCanvasSize)) error = "Invalid canvas value in [pack]";
            else if (key == "projection") pack_.projection = lower(value);
            else if (key == "name") pack_.name = value;
        } else if (section == Section::Map) {
            if (key == "id") current.id = value;
            else if (key == "name_ru") current.nameRu = value;
            else if (key == "name_en") current.nameEn = value;
            else if (key == "description_ru") current.descriptionRu = value;
            else if (key == "description_en") current.descriptionEn = value;
            else if (key == "credit") current.credit = value;
            else if (key == "file") current.path = joinPath(dir, value);
            else if (key == "kind") mapUsable = lower(value) == "base";
            else if (key == "world_left") parseFloatStrict(value, current.left);
            else if (key == "world_right") parseFloatStrict(value, current.right);
            else if (key == "world_top") parseFloatStrict(value, current.top);
            else if (key == "world_bottom") parseFloatStrict(value, current.bottom);
        }
        if (!error.empty()) break;
    }
    if (error.empty()) commitMap();
    if (error.empty() && !hasCanvasSize) pack_.canvasSize = legacyCanvasSize;
    if (!error.empty()) { maps_.clear(); pack_ = {}; return false; }
    if (pack_.format != kMapPackFormat) { error = "Unsupported map-pack format (expected 1)"; maps_.clear(); return false; }
    if (pack_.projection != kProjection) { error = "Unsupported projection (expected sa-world-v1)"; maps_.clear(); return false; }
    if (pack_.canvasSize < 512 || pack_.canvasSize > 4096) { error = "canvas_size must be 512..4096"; maps_.clear(); return false; }
    if (maps_.empty()) { error = "maps.ini contains no usable [map] entries"; return false; }
    return true;
}

bool MapTexture::discover(std::string& status) {
    maps_.clear();
    pack_ = {};
    index_ = -1;
    const std::string manifestPath = std::string(kAppDir) + "/maps/maps.ini";
    std::string error;
    if (!parseManifest(manifestPath, error)) {
        status = "Map pack unavailable: " + error;
        return false;
    }
    status = "Map pack: " + std::to_string(maps_.size()) + " map(s), " + pack_.projection;
    return true;
}

bool MapTexture::tryLoad(SDL_Renderer* renderer, const std::string& path, std::string& error) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) { error = IMG_GetError(); return false; }
    if (surface->w != surface->h || surface->w != pack_.canvasSize || surface->h != pack_.canvasSize) {
        std::ostringstream ss;
        ss << "Map must be " << pack_.canvasSize << 'x' << pack_.canvasSize << "; got " << surface->w << 'x' << surface->h;
        error = ss.str();
        SDL_FreeSurface(surface);
        return false;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { error = SDL_GetError(); SDL_FreeSurface(surface); return false; }
    unload();
    texture_ = texture;
    width_ = surface->w;
    height_ = surface->h;
    source_ = path;
    SDL_FreeSurface(surface);
    return true;
}

bool MapTexture::loadIndex(SDL_Renderer* renderer, int index, std::string& status) {
    if (maps_.empty()) return false;
    const int n = static_cast<int>(maps_.size());
    index = (index % n + n) % n;
    for (int attempt = 0; attempt < n; ++attempt) {
        const int candidate = (index + attempt) % n;
        std::string error;
        if (tryLoad(renderer, maps_[static_cast<std::size_t>(candidate)].path, error)) {
            index_ = candidate;
            status = "Map: " + maps_[static_cast<std::size_t>(candidate)].id;
            return true;
        }
        status = "Skipped map " + maps_[static_cast<std::size_t>(candidate)].id + ": " + error;
    }
    index_ = -1;
    return false;
}

bool MapTexture::discoverAndLoad(SDL_Renderer* renderer, const std::string& preferredId, std::string& status) {
    unload();
    if (!discover(status)) return false;
    int preferred = 0;
    for (std::size_t i = 0; i < maps_.size(); ++i) {
        if (!preferredId.empty() && maps_[i].id == preferredId) { preferred = static_cast<int>(i); break; }
    }
    return loadIndex(renderer, preferred, status);
}

bool MapTexture::loadFallback(SDL_Renderer* renderer, std::string& status) {
    SDL_RWops* rw = SDL_RWFromConstMem(fallback_map_bin, static_cast<int>(fallback_map_bin_size));
    if (!rw) { status = "Built-in map unavailable: " + std::string(SDL_GetError()); return false; }
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) { status = "Built-in map unavailable: " + std::string(IMG_GetError()); return false; }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { status = "Built-in map unavailable: " + std::string(SDL_GetError()); SDL_FreeSurface(surface); return false; }
    unload();
    texture_ = texture;
    width_ = surface->w;
    height_ = surface->h;
    fallback_ = true;
    source_ = "embedded Apache-2.0 SVG";
    SDL_FreeSurface(surface);
    status = "Map: built-in open SVG";
    return true;
}

bool MapTexture::cycle(SDL_Renderer* renderer, int delta, std::string& status) {
    if (maps_.empty()) { status = "No external map pack installed"; return false; }
    const int n = static_cast<int>(maps_.size());
    const int direction = delta < 0 ? -1 : 1;
    const int start = index_ >= 0 ? index_ : 0;
    for (int step = 1; step <= n; ++step) {
        const int candidate = (start + direction * step % n + n) % n;
        std::string error;
        if (tryLoad(renderer, maps_[static_cast<std::size_t>(candidate)].path, error)) {
            index_ = candidate;
            status = "Map: " + maps_[static_cast<std::size_t>(candidate)].id;
            return true;
        }
    }
    status = "No usable maps in map pack";
    return false;
}

SDL_Rect MapTexture::contentRect(const SDL_Rect& viewport) const {
    return viewport;
}

namespace {
struct SourceWindow { int x{}, y{}, w{}, h{}; };

SourceWindow sourceWindow(const MapView& input, int width, int height, const MapEntry& calibration,
                          const SDL_Rect& destination) {
    MapView view = input;
    clampMapView(view, 8.0f);
    const MapSourceSize sourceSize = mapSourceSize(width, height, view.zoom, destination.w, destination.h);
    // At zoomed views, a widescreen viewport exposes more world horizontally
    // while the vertical scale remains unchanged. At the outermost map edge
    // the finite raster naturally limits the available source rectangle.
    const int sw = sourceSize.width;
    const int sh = sourceSize.height;
    const float px = (view.centerX - calibration.left) * width / (calibration.right - calibration.left);
    const float py = (calibration.top - view.centerY) * height / (calibration.top - calibration.bottom);
    const int sx = std::clamp(static_cast<int>(std::lround(px - sw * 0.5)), 0, width - sw);
    const int sy = std::clamp(static_cast<int>(std::lround(py - sh * 0.5)), 0, height - sh);
    return SourceWindow{sx, sy, sw, sh};
}
} // namespace

bool MapTexture::render(SDL_Renderer* renderer, const MapView& inputView, const SDL_Rect& dst) const {
    if (!texture_ || width_ <= 0 || height_ <= 0 || dst.w <= 0 || dst.h <= 0) return false;
    const SDL_Rect content = contentRect(dst);
    const SourceWindow source = sourceWindow(inputView, width_, height_, activeCalibration(), content);
    const SDL_Rect src{source.x, source.y, source.w, source.h};
    return SDL_RenderCopy(renderer, texture_, &src, &content) == 0;
}

bool MapTexture::projectWorldPoint(const MapView& inputView, const SDL_Rect& dst,
                                   float x, float y, int& screenX, int& screenY) const {
    if (!texture_ || width_ <= 0 || height_ <= 0 || dst.w <= 0 || dst.h <= 0) return false;
    const SDL_Rect content = contentRect(dst);
    if (content.w <= 0 || content.h <= 0) return false;
    const MapEntry calibration = activeCalibration();
    const SourceWindow source = sourceWindow(inputView, width_, height_, calibration, content);
    const double pointX = (x - calibration.left) * width_ / (calibration.right - calibration.left);
    const double pointY = (calibration.top - y) * height_ / (calibration.top - calibration.bottom);
    screenX = content.x + static_cast<int>(std::lround((pointX - source.x) * content.w / source.w));
    screenY = content.y + static_cast<int>(std::lround((pointY - source.y) * content.h / source.h));
    return true;
}

bool MapTexture::screenToWorld(const MapView& inputView, const SDL_Rect& dst, int screenX, int screenY,
                               float& worldX, float& worldY) const {
    if (!texture_ || width_ <= 0 || height_ <= 0) return false;
    const SDL_Rect content = contentRect(dst);
    if (screenX < content.x || screenX >= content.x + content.w || screenY < content.y || screenY >= content.y + content.h) return false;
    const MapEntry calibration = activeCalibration();
    const SourceWindow source = sourceWindow(inputView, width_, height_, calibration, content);
    const float px = source.x + static_cast<float>(screenX - content.x) * source.w / content.w;
    const float py = source.y + static_cast<float>(screenY - content.y) * source.h / content.h;
    worldX = calibration.left + px * (calibration.right - calibration.left) / width_;
    worldY = calibration.top - py * (calibration.top - calibration.bottom) / height_;
    return true;
}

void MapTexture::panByScreenDelta(MapView& view, const SDL_Rect& dst, float dx, float dy) const {
    const SDL_Rect content = contentRect(dst);
    if (content.w <= 0 || content.h <= 0) return;
    view.centerX -= dx * 6000.0f / (view.zoom * content.w);
    view.centerY += dy * 6000.0f / (view.zoom * content.h);
    clampMapView(view, 8.0f);
}

} // namespace gtasa
