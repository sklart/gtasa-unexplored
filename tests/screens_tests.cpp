#include "FiltersScreen.hpp"
#include "ObjectListScreen.hpp"
#include <cassert>
#include <iostream>
int main() { using namespace gtasa; assert(filtersScreenLayout().count == 16); assert(nextFiltersScreenRow(0,-1)==15); assert(clampObjectListIndex(5,3)==2); assert(nextObjectListIndex(2,3,1)==0); assert((objectListRouteOrder({1,2,3},1)==std::vector<int>{2,3,1})); std::cout<<"screen tests passed\n"; }
