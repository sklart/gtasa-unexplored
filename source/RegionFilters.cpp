#include "RegionFilters.hpp"

namespace gtasa {

bool regionEnabled(const RegionFilters& filters, SanAndreasRegion region) {
    const auto index = static_cast<std::size_t>(region);
    return index < filters.size() && filters[index];
}

std::string encodeRegionFilters(const RegionFilters& filters) {
    std::string encoded;
    encoded.reserve(filters.size());
    for (const bool enabled : filters) encoded += enabled ? '1' : '0';
    return encoded;
}

bool decodeRegionFilters(const std::string& encoded, RegionFilters& filters) {
    if (encoded.size() != filters.size()) return false;
    RegionFilters decoded{};
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] != '0' && encoded[i] != '1') return false;
        decoded[i] = encoded[i] == '1';
    }
    filters = decoded;
    return true;
}

SanAndreasRegion regionForPoiCoordinate(float x, float y) {
    if (y < -700.0f) return SanAndreasRegion::LosSantos;
    if (x < -1000.0f && y >= 0.0f) return SanAndreasRegion::SanFierro;
    if (x > 650.0f && y >= 450.0f) return SanAndreasRegion::LasVenturas;
    return SanAndreasRegion::Countryside;
}

} // namespace gtasa
