#include "PoiRegions.hpp"

namespace gtasa {
namespace {
// 1..14 Los Santos, 15..28 San Fierro, 29..42 Las Venturas, 43..80 Countryside.
constexpr char kPoiRegionCatalogue[] =
    "LLLLLLLLLLLLLL"
    "SSSSSSSSSSSSSS"
    "VVVVVVVVVVVVVV"
    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC";
static_assert(sizeof(kPoiRegionCatalogue) == 81, "POI catalogue must contain all 80 POIs");
}

SanAndreasRegion regionForPoi(int poiId) {
    if (poiId < 1 || poiId > 80) return SanAndreasRegion::Count;
    switch (kPoiRegionCatalogue[poiId - 1]) {
        case 'L': return SanAndreasRegion::LosSantos;
        case 'S': return SanAndreasRegion::SanFierro;
        case 'V': return SanAndreasRegion::LasVenturas;
        case 'C': return SanAndreasRegion::Countryside;
        default: return SanAndreasRegion::Count;
    }
}

} // namespace gtasa
