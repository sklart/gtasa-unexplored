#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace gtasa {

// A missing preferred id intentionally selects the first available map.
inline int preferredMapIndex(const std::vector<std::string>& ids, const std::string& preferredId) {
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (!preferredId.empty() && ids[i] == preferredId) return static_cast<int>(i);
    }
    return 0;
}

} // namespace gtasa
