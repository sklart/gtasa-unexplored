#pragma once

namespace gtasa {

enum class OverlayKind { None, Filters, ObjectList, Details };

struct OverlayCloseArea {
    int x{};
    int y{};
    int width{};
    int height{};
};

// The symbol itself may be compact, but its hit target deliberately follows
// the platform touch-target guidance instead of the glyph's dimensions.
constexpr OverlayCloseArea overlayCloseArea(int screenWidth, int inset = 18, int touchSize = 48) {
    return {screenWidth - inset - touchSize, inset, touchSize, touchSize};
}

constexpr OverlayCloseArea overlayCloseAreaInBounds(int x, int y, int width, int inset = 10,
                                                     int touchSize = 48) {
    return {x + width - inset - touchSize, y + inset, touchSize, touchSize};
}

constexpr bool hitsOverlayClose(const OverlayCloseArea& area, int x, int y) {
    return x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height;
}

constexpr OverlayKind activeOverlay(bool filtersOpen, bool listOpen, bool detailsOpen) {
    return detailsOpen ? OverlayKind::Details : listOpen ? OverlayKind::ObjectList
           : filtersOpen ? OverlayKind::Filters : OverlayKind::None;
}

} // namespace gtasa
