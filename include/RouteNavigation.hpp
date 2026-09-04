#pragma once

#include <vector>

namespace gtasa {

int normalizeRouteIndex(int index, int count);
int nextRouteIndex(int current, int count);
std::vector<int> orderedRouteIndices(const std::vector<int>& route, int currentIndex);

} // namespace gtasa
