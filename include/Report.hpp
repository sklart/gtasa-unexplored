#pragma once

#include "Collectibles.hpp"
#include <string>

namespace gtasa {

// Stable machine-readable parser report shared by the Switch diagnostic export
// and the host-side inspect-save utility.
std::string parseResultJson(const ParseResult& result, bool includeObjects = true);

} // namespace gtasa
