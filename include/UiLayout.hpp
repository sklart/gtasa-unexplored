#pragma once

#include <string>

// Shared presentation measurements for the 1280x720 Switch canvas.  Keeping
// these values here prevents one-off offsets from gradually drifting between
// the sidebar and overlays.
namespace gtasa {
constexpr int kUiScreenWidth = 1280;
constexpr int kUiScreenHeight = 720;
constexpr int kUiPanelWidth = 390;
constexpr int kUiPanelPaddingX = 26;
constexpr int kUiPanelPaddingY = 22;
constexpr int kUiSectionGap = 14;
constexpr int kUiRowGap = 7;
constexpr int kUiTitleGap = 9;
constexpr int kUiSidebarControlsTop = 580;

struct UiTextRect { int x{}; int y{}; int width{}; int height{}; };
struct UiRect { int x{}; int y{}; int width{}; int height{}; };

// Callers provide the actual SDL_ttf measurement.  This makes the sidebar
// layout testable without assuming an English-only glyph width.
constexpr bool textFitsRect(const UiTextRect& text, const UiTextRect& bounds) {
    return text.x >= bounds.x && text.y >= bounds.y &&
           text.x + text.width <= bounds.x + bounds.width &&
           text.y + text.height <= bounds.y + bounds.height;
}

constexpr bool twoColumnRowFits(int panelContentWidth, int leftMeasuredWidth,
                                int rightMeasuredWidth, int gap = 12) {
    return leftMeasuredWidth >= 0 && rightMeasuredWidth >= 0 &&
           leftMeasuredWidth + gap + rightMeasuredWidth <= panelContentWidth;
}

constexpr bool rowsDoNotOverlap(int firstY, int firstHeight, int secondY) {
    return secondY >= firstY + firstHeight;
}

constexpr bool hitsRect(const UiRect& rect, int x, int y) {
    return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height;
}

constexpr UiRect regionTableButtonRect(int panelRight, int titleY, int width = 88, int height = 28) {
    return {panelRight - width, titleY - 4, width, height};
}

// Remove exactly one complete UTF-8 code point. Invalid trailing bytes are
// removed as a unit as well, so truncation never leaves a partial multibyte
// character behind.
inline bool popUtf8CodePoint(std::string& value) {
    if (value.empty()) return false;
    std::size_t end = value.size();
    do { --end; } while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xc0) == 0x80);
    value.erase(end);
    return true;
}

inline bool isValidUtf8(const std::string& value) {
    for (std::size_t i = 0; i < value.size();) {
        const unsigned char lead = static_cast<unsigned char>(value[i]);
        const int length = lead < 0x80 ? 1 : (lead >= 0xc2 && lead <= 0xdf) ? 2
            : (lead >= 0xe0 && lead <= 0xef) ? 3 : (lead >= 0xf0 && lead <= 0xf4) ? 4 : 0;
        if (length == 0 || i + static_cast<std::size_t>(length) > value.size()) return false;
        for (int byte = 1; byte < length; ++byte)
            if ((static_cast<unsigned char>(value[i + static_cast<std::size_t>(byte)]) & 0xc0) != 0x80) return false;
        i += static_cast<std::size_t>(length);
    }
    return true;
}

template <typename Fits>
std::string ellipsizeUtf8(std::string value, Fits fits) {
    if (fits(value)) return value;
    const std::string suffix = "…";
    while (popUtf8CodePoint(value)) {
        if (fits(value + suffix)) return value + suffix;
    }
    return fits(suffix) ? suffix : std::string{};
}
}
