#include "CollectibleIcons.hpp"
#include "MapProjection.hpp"
#include "MarkerSelection.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <utility>

int main() {
    using namespace gtasa;
    assert(collectibleIconSize(0.85f) >= 28);
    assert(collectibleIconSize(1.0f) == 28);
    assert(collectibleIconSize(8.0f) == 42);
    assert(mapSourceSize(2048, 2048, 2.0f, 1280, 720).width >
           mapSourceSize(2048, 2048, 2.0f, 955, 720).width);
    int previous = collectibleIconSize(0.85f);
    for (float zoom = 1.0f; zoom <= 8.0f; zoom += 0.1f) {
        const int size = collectibleIconSize(zoom);
        assert(size >= previous && size >= 28 && size <= 42);
        previous = size;
    }
    for (const auto& viewport : {std::pair<int, int>{955, 720}, {1280, 720}}) {
        for (const float zoom : {1.0f, 2.0f, 8.0f}) {
            const auto source = mapSourceSize(2048, 2048, zoom, viewport.first, viewport.second);
            const auto destination = mapDestinationSize(source, viewport.first, viewport.second);
            const float destinationX = (viewport.first - destination.width) * 0.5f;
            const float destinationY = (viewport.second - destination.height) * 0.5f;
            const float pixelsPerSourceX = static_cast<float>(destination.width) / source.width;
            const float pixelsPerSourceY = static_cast<float>(destination.height) / source.height;
            assert(std::fabs(pixelsPerSourceX - pixelsPerSourceY) < 0.001f);
            const float sx = mapWorldToScreenAxis(350.0f, 320.0f, static_cast<float>(source.width),
                                                  -3000.0f, 3000.0f, 2048.0f, destinationX, static_cast<float>(destination.width));
            const float world = mapScreenToWorldAxis(sx, destinationX, static_cast<float>(destination.width),
                                                     320.0f, static_cast<float>(source.width),
                                                     -3000.0f, 3000.0f, 2048.0f);
            assert(std::fabs(world - 350.0f) < 0.01f);
            const float sy = mapWorldToScreenAxis(350.0f, 320.0f, static_cast<float>(source.height),
                                                  -3000.0f, 3000.0f, 2048.0f, destinationY, static_cast<float>(destination.height));
            const float worldY = mapScreenToWorldAxis(sy, destinationY, static_cast<float>(destination.height),
                                                       320.0f, static_cast<float>(source.height),
                                                       -3000.0f, 3000.0f, 2048.0f);
            assert(std::fabs(worldY - 350.0f) < 0.01f);
            const float panX = 37.0f * 6000.0f * source.width / (2048.0f * destination.width);
            const float panY = 37.0f * 6000.0f * source.height / (2048.0f * destination.height);
            // Integer SDL rectangles can differ by less than one source pixel;
            // the resulting world delta must remain visually isotropic.
            assert(std::fabs(panX - panY) < 0.1f);
        }
    }
    const float calibratedScreen = mapWorldToScreenAxis(175.0f, 100.0f, 800.0f,
                                                        -1000.0f, 1000.0f, 2048.0f, 0.0f, 1280.0f);
    const float calibratedWorld = mapScreenToWorldAxis(calibratedScreen, 0.0f, 1280.0f,
                                                        100.0f, 800.0f, -1000.0f, 1000.0f, 2048.0f);
    assert(std::fabs(calibratedWorld - 175.0f) < 0.01f);
    const std::vector<MarkerSelectionPoint> overlapping{{1, 100.0f, 100.0f}, {2, 111.0f, 105.0f},
                                                         {3, 118.0f, 103.0f}, {4, 190.0f, 100.0f}};
    int selected = 0;
    assert(cycleOverlappingMarker(overlapping, 1, 1, 24.0f, selected) && selected == 2);
    assert(cycleOverlappingMarker(overlapping, selected, 1, 24.0f, selected) && selected == 3);
    assert(cycleOverlappingMarker(overlapping, selected, 1, 24.0f, selected) && selected == 1);
    assert(cycleOverlappingMarker(overlapping, selected, -1, 24.0f, selected) && selected == 3);
    assert(!cycleOverlappingMarker(overlapping, 4, 1, 24.0f, selected));
    const std::vector<bool> visible{true, true, true, true};
    // Global L3/R3 navigation remains independent from the overlap group.
    assert(nextVisibleMarkerIndex(1, 4, 1, [&](int index) { return visible[index]; }) == 2);
    assert(nextVisibleMarkerIndex(1, 4, -1, [&](int index) { return visible[index]; }) == 0);
    std::cout << "map UI tests passed\n";
}
