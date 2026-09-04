#pragma once

#include <cmath>

namespace gtasa {

struct CameraCenter { float x{}; float y{}; };
enum class CameraOwner { Cursor, Touch };
struct CameraComfortZone { float halfWidth{}; float halfHeight{}; };

inline void moveCursorToSelectedMarker(float& cursorX, float& cursorY, CameraOwner& owner,
                                       float markerX, float markerY) {
    cursorX = markerX;
    cursorY = markerY;
    owner = CameraOwner::Cursor;
}

inline bool cursorOutsideComfortZone(const CameraCenter& camera, float cursorX, float cursorY,
                                     CameraComfortZone comfort) {
    return std::abs(cursorX - camera.x) > comfort.halfWidth ||
           std::abs(cursorY - camera.y) > comfort.halfHeight;
}

inline bool focusSelectedMarker(CameraCenter& camera, float& cursorX, float& cursorY,
                                CameraOwner& owner, float markerX, float markerY,
                                CameraComfortZone comfort) {
    moveCursorToSelectedMarker(cursorX, cursorY, owner, markerX, markerY);
    if (!cursorOutsideComfortZone(camera, cursorX, cursorY, comfort)) return false;
    camera.x = cursorX;
    camera.y = cursorY;
    return true;
}

inline void updateCameraForCursor(CameraCenter& camera, float cursorX, float cursorY,
                                  bool cursorMoved, CameraOwner& owner,
                                  CameraComfortZone comfort) {
    if (cursorMoved) owner = CameraOwner::Cursor;
    if (owner == CameraOwner::Cursor &&
        (cursorMoved || cursorOutsideComfortZone(camera, cursorX, cursorY, comfort))) {
        camera.x = cursorX;
        camera.y = cursorY;
    }
}

} // namespace gtasa
