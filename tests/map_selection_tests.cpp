#include "MapSelection.hpp"
#include "MapUi.hpp"
#include "UiStatus.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
    const std::vector<std::string> maps{"classic", "definitive", "satellite"};

    // Existing preferred id is selected exactly.
    assert(gtasa::preferredMapIndex(maps, "definitive") == 1);

    // A disappeared id falls back to the first available map and that actual
    // id is saved back to config by persistCurrentMap().
    const int fallback = gtasa::preferredMapIndex(maps, "removed-map");
    assert(fallback == 0);
    assert(gtasa::shouldPersistMapId("removed-map", maps[static_cast<std::size_t>(fallback)]));

    assert(gtasa::localizeSystemStatus("map.loaded", true) == "Карта загружена");
    assert(gtasa::localizeSystemStatus("map.loaded", false) == "Map loaded");
    return 0;
}
