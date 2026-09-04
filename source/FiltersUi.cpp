#include "FiltersUi.hpp"
namespace gtasa { int nextFilterRow(int current, int direction) { const int n=filtersUiLayout().count; return (current + direction % n + n) % n; } }
