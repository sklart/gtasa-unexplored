#pragma once
#include "FiltersUi.hpp"
namespace gtasa {
// UI-independent filter screen navigation/layout used by SDL rendering.
using FiltersScreenLayout = FiltersUiLayout;
constexpr FiltersScreenLayout filtersScreenLayout() { return filtersUiLayout(); }
int nextFiltersScreenRow(int current, int direction);
} // namespace gtasa
