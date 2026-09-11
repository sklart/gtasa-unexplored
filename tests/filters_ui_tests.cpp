#include "FiltersUi.hpp"
#include <cassert>
#include <iostream>
int main() { using namespace gtasa; const auto l=filtersUiLayout(); assert(l.count==16); assert(l.modeRow==0); assert(l.collectibleFirst==1); assert(l.regionFirst==6); assert(l.poiRow==10); assert(nextFilterRow(0,-1)==15); assert(nextFilterRow(15,1)==0); assert(nextCollectibleViewMode(0)==1); assert(nextCollectibleViewMode(1)==2); assert(nextCollectibleViewMode(2)==0); std::cout<<"filters UI tests passed\n"; }
