#include "ObjectListScreen.hpp"
namespace gtasa {
int clampObjectListIndex(int index, int count) { return count > 0 ? (index < 0 ? 0 : (index >= count ? count - 1 : index)) : 0; }
int nextObjectListIndex(int index, int count, int direction) { return count > 0 ? normalizeRouteIndex(index + direction, count) : 0; }
std::vector<int> objectListRouteOrder(const std::vector<int>& route, int routeIndex) { return orderedRouteIndices(route, routeIndex); }
} // namespace gtasa
