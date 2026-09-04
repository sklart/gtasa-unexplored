#include "PoiCategories.hpp"

namespace gtasa {

const char* poiCategoryName(PoiCategory category, bool russian) {
    switch (category) {
        case PoiCategory::Story: return russian ? "Сюжет" : "Story";
        case PoiCategory::Landmark: return russian ? "Достопримечательности" : "Landmarks";
        case PoiCategory::Nature: return russian ? "Природа" : "Nature";
        case PoiCategory::Mystery: return russian ? "Тайны" : "Mysteries";
        case PoiCategory::Business: return russian ? "Заведения" : "Businesses";
        default: return russian ? "Неизвестно" : "Unknown";
    }
}

bool poiCategoryEnabled(const PoiCategoryFilters& filters, PoiCategory category) {
    const auto index = static_cast<std::size_t>(category);
    return index < filters.size() && filters[index];
}

std::string encodePoiCategoryFilters(const PoiCategoryFilters& filters) {
    std::string encoded;
    encoded.reserve(filters.size());
    for (const bool enabled : filters) encoded += enabled ? '1' : '0';
    return encoded;
}

bool decodePoiCategoryFilters(const std::string& encoded, PoiCategoryFilters& filters) {
    if (encoded.size() != filters.size()) return false;
    PoiCategoryFilters decoded{};
    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] != '0' && encoded[i] != '1') return false;
        decoded[i] = encoded[i] == '1';
    }
    filters = decoded;
    return true;
}

} // namespace gtasa
