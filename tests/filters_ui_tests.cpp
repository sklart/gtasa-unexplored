#include "FiltersUi.hpp"
#include <cassert>
#include <iostream>
int main() { using namespace gtasa; const auto l=filtersUiLayout(); assert(l.count==16); assert(nextFilterRow(0,-1)==15); assert(nextFilterRow(15,1)==0); std::cout<<"filters UI tests passed\n"; }
