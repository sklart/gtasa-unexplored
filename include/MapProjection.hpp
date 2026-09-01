#pragma once

#include <algorithm>
#include <cmath>

namespace gtasa {

struct MapSourceSize { int width{}; int height{}; };

inline MapSourceSize mapSourceSize(int canvasWidth, int canvasHeight, float zoom,
                                   int viewportWidth, int viewportHeight) {
    const int height = std::clamp(static_cast<int>(std::lround(canvasHeight / zoom)), 1, canvasHeight);
    const double aspect = viewportHeight > 0 ? static_cast<double>(viewportWidth) / viewportHeight : 1.0;
    return {std::clamp(static_cast<int>(std::lround(height * aspect)), 1, canvasWidth), height};
}

inline float mapScreenToWorldAxis(float screen, float viewportStart, float viewportSize,
                                  float sourceStart, float sourceSize, float worldStart, float worldEnd,
                                  float canvasSize) {
    const float pixel = sourceStart + (screen - viewportStart) * sourceSize / viewportSize;
    return worldStart + pixel * (worldEnd - worldStart) / canvasSize;
}

inline float mapWorldToScreenAxis(float world, float sourceStart, float sourceSize,
                                  float worldStart, float worldEnd, float canvasSize,
                                  float viewportStart, float viewportSize) {
    const float pixel = (world - worldStart) * canvasSize / (worldEnd - worldStart);
    return viewportStart + (pixel - sourceStart) * viewportSize / sourceSize;
}

} // namespace gtasa
