#include "Platform.hpp"

#include <cassert>
#include <cstdio>

int main() {
    constexpr const char* path = "platform-config-roundtrip.ini";
    std::remove(path);

    gtasa::Platform platform;
    gtasa::AppConfig saved;
    saved.language = "en";
    saved.mapId = "definitive";
    saved.preferredSlot = 4;
    saved.showPoi = false;
    saved.collectibleViewMode = 2;
    assert(platform.saveConfigFile(saved, path));

    const auto loaded = platform.loadConfigFile(path);
    assert(loaded.language == "en");
    assert(loaded.mapId == "definitive");
    assert(loaded.preferredSlot == 4);
    assert(!loaded.showPoi);
    assert(loaded.collectibleViewMode == 2);
    std::remove(path);
    return 0;
}
