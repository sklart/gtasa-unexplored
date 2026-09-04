#pragma once

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
    if (zoom <= 0.85f) return 28;
    if (zoom < 1.75f) return 28 + static_cast<int>((zoom - 0.85f) * 4.0f / 0.90f + 0.5f);
    if (zoom >= 8.0f) return 37;
    return 32 + static_cast<int>((zoom - 1.75f) * 5.0f / 6.25f + 0.5f);
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
