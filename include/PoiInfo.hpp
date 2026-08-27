#pragma once

#include <cstddef>

namespace gtasa {

struct PoiInfo {
    int id;
    float x, y, z;
    bool visibleOnMap;
    bool representative;
    const char* nameEn;
    const char* nameRu;
    const char* descriptionEn;
    const char* descriptionRu;
    const char* imagePath; // Relative to sdmc:/switch/gtasa-unexplored/poi/.
};

const PoiInfo* poiInfo(std::size_t index);
std::size_t poiInfoCount();

} // namespace gtasa
