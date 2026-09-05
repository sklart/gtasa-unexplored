#include "OverlayUi.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    constexpr auto close = overlayCloseArea(1280);
    static_assert(close.width >= 44 && close.height >= 44);
    assert(hitsOverlayClose(close, close.x, close.y));
    assert(hitsOverlayClose(close, close.x + close.width - 1, close.y + close.height - 1));
    assert(!hitsOverlayClose(close, close.x - 1, close.y));
    assert(!hitsOverlayClose(close, close.x, close.y + close.height));
    assert(!hitsOverlayClose(close, 0, 0));
    constexpr auto localClose = overlayCloseAreaInBounds(80, 22, 1120);
    assert(hitsOverlayClose(localClose, 1151, 32));
    assert(!hitsOverlayClose(localClose, 1200, 32));
    assert(activeOverlay(false, false, false) == OverlayKind::None);
    assert(activeOverlay(true, false, false) == OverlayKind::Filters);
    assert(activeOverlay(true, true, false) == OverlayKind::ObjectList);
    assert(activeOverlay(true, true, true) == OverlayKind::Details);
    assert(activeOverlay(false, false, false, true) == OverlayKind::RegionProgress);
    std::cout << "overlay UI tests passed\n";
}
