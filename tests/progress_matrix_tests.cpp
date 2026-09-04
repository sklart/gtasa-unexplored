#include "ProgressMatrix.hpp"
#include "CollectibleView.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    ParseResult result;
    result.ok = true;
    assert(buildCollectibleObjects(result));
    const auto matrix = calculateProgressMatrix(result);
    int total = 0, completed = 0;
    for (const auto& region : matrix) for (const auto& cell : region) { total += cell.total; completed += cell.completed; }
    assert(total == 320 && completed == 320);
    assert(matrix[static_cast<std::size_t>(SanAndreasRegion::LosSantos)][static_cast<std::size_t>(CollectibleType::Tag)].total == 100);
    std::cout << "progress matrix tests passed\n";
}
