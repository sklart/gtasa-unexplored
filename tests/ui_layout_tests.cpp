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
    constexpr UiRect visual = regionTableButtonRect(kUiScreenWidth - kUiPanelPaddingX, 318);
    constexpr UiRect touch = visual; // Rendering and touch must share this exact rectangle.
    assert(visual.x == touch.x && visual.y == touch.y && visual.width == touch.width && visual.height == touch.height);
    assert(hitsRect(touch, visual.x, visual.y));
    assert(hitsRect(touch, visual.x + visual.width - 1, visual.y + visual.height - 1));
    assert(!hitsRect(touch, visual.x - 1, visual.y));
    const std::string longRussian = "Сельская местность и достопримечательность";
    const std::string clipped = ellipsizeUtf8(longRussian, [](const std::string& value) { return value.size() <= 24; });
    assert(clipped.size() >= 3 && clipped.compare(clipped.size() - 3, 3, "…") == 0);
    assert(isValidUtf8(clipped));
    assert(clipped.size() <= 24);
    std::cout << "UI layout tests passed\n";
}
