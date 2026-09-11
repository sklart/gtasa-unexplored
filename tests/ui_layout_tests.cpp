#include "UiLayout.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace gtasa;
    constexpr int content = kUiPanelWidth - 2 * kUiPanelPaddingX;
    // Measured-equivalent widths for the longest current RU/EN sidebar labels.
    assert(twoColumnRowFits(content, 188, 52)); // «Сельская местность» + 100/149
    assert(twoColumnRowFits(content, 142, 48)); // "Countryside" + 100/149
    assert(!twoColumnRowFits(content, content - 20, 40));
    assert(rowsDoNotOverlap(316, 19, 342));
    assert(!rowsDoNotOverlap(316, 24, 332));
    constexpr UiTextRect panel{kUiScreenWidth - kUiPanelWidth + kUiPanelPaddingX,
                               kUiPanelPaddingY, content, kUiScreenHeight - 2 * kUiPanelPaddingY};
    assert(textFitsRect({panel.x, panel.y, 180, 28}, panel));
    assert(!textFitsRect({panel.x + content - 80, panel.y, 120, 28}, panel));
    std::cout << "UI layout tests passed\n";
}
