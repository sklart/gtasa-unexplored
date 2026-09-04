#include "RouteNavigation.hpp"
#include <cassert>
#include <iostream>
int main() {
    using namespace gtasa;
    const std::vector<int> route{7, 2, 11};
    assert((orderedRouteIndices(route, 0) == route));
    assert((orderedRouteIndices(route, 1) == std::vector<int>{2, 11, 7}));
    assert(nextRouteIndex(0, 3) == 1);
    assert(nextRouteIndex(2, 3) == 0);
    assert(orderedRouteIndices({}, 0).empty());
    std::cout << "route navigation tests passed\n";
}
