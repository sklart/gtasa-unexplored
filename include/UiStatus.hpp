#pragma once

#include <string>

namespace gtasa {

// Platform and map code return stable, non-user-facing status keys. Keeping
// their translation here makes every visible message follow the UI language.
inline std::string localizeSystemStatus(const std::string& key, bool russian) {
    if (key == "save.no_profile") return russian ? "Профиль не выбран" : "No profile selected";
    if (key == "save.no_game_save") return russian ? "Для выбранного профиля нет сохранения GTA San Andreas DE"
                                                       : "No GTA San Andreas DE save exists for the selected profile";
    if (key == "save.backup_unavailable") return russian ? "Игра запущена, а резервной копии сохранения ещё нет. Закройте игру и один раз запустите GTASA Unexplored."
                                                            : "The game is running and no save snapshot exists yet. Close the game and run GTASA Unexplored once.";
    if (key == "save.not_found") return russian ? "Не удалось найти файлы сохранения GTA San Andreas DE"
                                                   : "Could not find GTA San Andreas DE save files";
    if (key == "map.pack_unavailable") return russian ? "Пакет карт недоступен; используется встроенная карта"
                                                          : "Map pack unavailable; using the built-in map";
    if (key == "map.loaded") return russian ? "Карта загружена" : "Map loaded";
    if (key == "map.fallback_loaded") return russian ? "Загружена встроенная открытая карта"
                                                        : "Built-in open map loaded";
    if (key == "map.no_external_pack") return russian ? "Внешний пакет карт не установлен"
                                                        : "No external map pack is installed";
    if (key == "map.no_usable_maps") return russian ? "В пакете нет пригодных для загрузки карт"
                                                       : "The map pack has no usable maps";
    if (key == "map.fallback_unavailable") return russian ? "Встроенная карта недоступна"
                                                             : "Built-in map unavailable";
    return {};
}

} // namespace gtasa
