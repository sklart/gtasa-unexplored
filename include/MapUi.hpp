#pragma once

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace gtasa {

constexpr float kTouchDragThreshold = 8.0f;

inline bool exceedsTouchDragThreshold(float startX, float startY, float x, float y) {
    const float dx = x - startX;
    const float dy = y - startY;
    return dx * dx + dy * dy >= kTouchDragThreshold * kTouchDragThreshold;
}

inline bool isTapGesture(bool moved, bool multiTouch) {
    return !moved && !multiTouch;
}

inline bool canResetMapFromDoubleTap(bool isTap, bool hitMarker) {
    return isTap && !hitMarker;
}

inline int collectibleIconSize(float zoom) {
    if (zoom <= 0.85f) return 25;
    if (zoom < 1.75f) return 25 + static_cast<int>((zoom - 0.85f) * 3.0f / 0.90f + 0.5f);
    if (zoom >= 8.0f) return 42;
    return 28 + static_cast<int>((zoom - 1.75f) * 14.0f / 6.25f + 0.5f);
}

inline int poiMarkerSize(float zoom) {
    // POI art is visually denser than the collectible pictograms.  Keep it
    // subordinate at a distant view, then grow it smoothly for close reading.
    if (zoom <= 0.85f) return 20;
    if (zoom < 1.75f) return 20 + static_cast<int>((zoom - 0.85f) * 5.0f / 0.90f + 0.5f);
    if (zoom >= 8.0f) return 33;
    return 25 + static_cast<int>((zoom - 1.75f) * 8.0f / 6.25f + 0.5f);
}

inline bool markerFullyVisible(int anchorX, int anchorY, int width, int height,
                               int viewportX, int viewportY, int viewportWidth, int viewportHeight,
                               bool anchorBottom = false) {
    const int left = anchorX - width / 2;
    const int top = anchorBottom ? anchorY - height : anchorY - height / 2;
    return left >= viewportX && top >= viewportY &&
           left + width <= viewportX + viewportWidth &&
           top + height <= viewportY + viewportHeight;
}

inline float markerVisibleFraction(int anchorX, int anchorY, int width, int height,
                                   int viewportX, int viewportY, int viewportWidth, int viewportHeight,
                                   bool anchorBottom = false) {
    const int left = anchorX - width / 2;
    const int top = anchorBottom ? anchorY - height : anchorY - height / 2;
    const int right = left + width;
    const int bottom = top + height;
    const int visibleW = std::max(0, std::min(right, viewportX + viewportWidth) - std::max(left, viewportX));
    const int visibleH = std::max(0, std::min(bottom, viewportY + viewportHeight) - std::max(top, viewportY));
    return width > 0 && height > 0 ? static_cast<float>(visibleW * visibleH) / (width * height) : 0.0f;
}

inline bool hasMarkerVisibleThreshold(int anchorX, int anchorY, int width, int height,
                                      int viewportX, int viewportY, int viewportWidth, int viewportHeight,
                                      bool anchorBottom = false, float threshold = 0.45f) {
    return markerVisibleFraction(anchorX, anchorY, width, height, viewportX, viewportY,
                                 viewportWidth, viewportHeight, anchorBottom) >= threshold;
}

inline const char* poiLocationStatus(bool representative, bool russian) {
    if (representative) return russian ? "Ориентировочно" : "Approximate";
    return russian ? "Проверено" : "Verified";
}

inline std::string formatMapCoordinates(float x, float y, float z, bool zKnown = true) {
    std::ostringstream value;
    value << std::fixed << std::setprecision(1)
          << "X " << x << "   Y " << y << "   Z ";
    if (zKnown) value << z;
    else value << "—";
    return value.str();
}

} // namespace gtasa
