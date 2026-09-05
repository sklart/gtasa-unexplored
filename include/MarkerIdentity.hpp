#pragma once

namespace gtasa {

enum class MarkerKind { None, Collectible, Poi };

struct MarkerIdentity {
    MarkerKind kind{MarkerKind::None};
    int index{-1};
};

constexpr bool operator==(MarkerIdentity left, MarkerIdentity right) {
    return left.kind == right.kind && left.index == right.index;
}
constexpr bool operator!=(MarkerIdentity left, MarkerIdentity right) { return !(left == right); }
constexpr bool hasMarker(MarkerIdentity marker) {
    return marker.kind != MarkerKind::None && marker.index >= 0;
}
constexpr MarkerIdentity collectibleMarker(int index) { return {MarkerKind::Collectible, index}; }
constexpr MarkerIdentity poiMarker(int index) { return {MarkerKind::Poi, index}; }

enum class MarkerTapAction { None, Select, OpenDetails };

// A tap has a single identity on both sides of the comparison: switching
// between a POI and collectible can therefore never look like a repeat tap.
constexpr MarkerTapAction markerTapAction(MarkerIdentity selected, MarkerIdentity hit) {
    if (!hasMarker(hit)) return MarkerTapAction::None;
    return selected == hit ? MarkerTapAction::OpenDetails : MarkerTapAction::Select;
}

} // namespace gtasa
