#pragma once

#include <cmath>

namespace gtasa {

struct CameraCenter { float x{}; float y{}; };
enum class CameraOwner { Cursor, Touch };

inline void moveCursorToSelectedMarker(float& cursorX, float& cursorY, CameraOwner& owner,
                                       float markerX, float markerY) {
    cursorX = markerX;
    cursorY = markerY;
    owner = CameraOwner::Cursor;
}

inline bool cursorOutsideComfortZone(const CameraCenter& camera, float cursorX, float cursorY,
                                     float comfortHalfWidth, float comfortHalfHeight) {
    return std::abs(cursorX - camera.x) > comfortHalfWidth ||
           std::abs(cursorY - camera.y) > comfortHalfHeight;
}

inline void updateCameraForCursor(CameraCenter& camera, float cursorX, float cursorY,
                                  bool cursorMoved, CameraOwner& owner,
                                  float comfortHalfWidth, float comfortHalfHeight) {
    if (cursorMoved) owner = CameraOwner::Cursor;
    if (owner == CameraOwner::Cursor &&
        (cursorMoved || cursorOutsideComfortZone(camera, cursorX, cursorY, comfortHalfWidth, comfortHalfHeight))) {
        camera.x = cursorX;
        camera.y = cursorY;
    }
}

} // namespace gtasa
