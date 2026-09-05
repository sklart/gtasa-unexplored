#include "MarkerIdentity.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    const auto tag = collectibleMarker(7);
    const auto poi = poiMarker(7);
    const auto otherPoi = poiMarker(8);
    assert(markerTapAction({}, tag) == MarkerTapAction::Select);
    assert(markerTapAction(tag, tag) == MarkerTapAction::OpenDetails);
    assert(markerTapAction(tag, poi) == MarkerTapAction::Select);
    assert(markerTapAction(poi, poi) == MarkerTapAction::OpenDetails);
    assert(markerTapAction(poi, tag) == MarkerTapAction::Select);
    assert(markerTapAction(poi, otherPoi) == MarkerTapAction::Select);
    assert(markerTapAction(tag, {}) == MarkerTapAction::None);
    std::cout << "marker identity tests passed\n";
}
