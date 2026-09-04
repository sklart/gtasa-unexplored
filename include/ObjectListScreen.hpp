#pragma once
#include "RouteNavigation.hpp"
#include <vector>
namespace gtasa {
int clampObjectListIndex(int index, int count);
int nextObjectListIndex(int index, int count, int direction);
std::vector<int> objectListRouteOrder(const std::vector<int>& route, int routeIndex);
} // namespace gtasa
