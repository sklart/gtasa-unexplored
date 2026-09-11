#pragma once

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
constexpr int kUiSidebarControlsTop = 566;

struct UiTextRect { int x{}; int y{}; int width{}; int height{}; };

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
}
