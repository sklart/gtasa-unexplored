# Research and third-party notes

GTASA Unexplored — независимое homebrew-приложение. Source/release package намеренно не содержит Rockstar/Fandom map imagery.

Встроенная fallback-карта — производная от `map.svg` из
[`Toliak/San-Andreas-vector-map`](https://github.com/Toliak/San-Andreas-vector-map),
лицензия Apache-2.0. Неизменённый исходник и текст лицензии находятся в
`third_party/toliak-san-andreas-vector-map/`; в NRO встраивается его копия с
тем же viewBox, но 2048×2048 intrinsic canvas для безопасного использования в
applet mode.

Для исследования save semantics и проверки фактических мировых координат использовались публичные проекты/материалы, включая:

- `gta-reversed/gta-reversed` — структуры и gameplay semantics;
- `gtasa-savegame-editor/gtasa-savegame-editor` — проверка save-array order Tags;
- `user-grinch/GrinchTrainer-III-VC-SA` — исторический coordinate dataset для cross-check;
- `interactive-game-maps/grand_theft_auto_san_andreas` — независимая coordinate cross-check.

`TagData.cpp` и `CollectibleData.cpp` содержат нормализованные фактические игровые world coordinates, а не код интерфейса или графические ассеты этих проектов. Подробности и fail-closed правила описаны в `docs/COLLECTIBLE_DATA.md`.

Отдельно подготовленные map-pack'и могут содержать пользовательские или отдельно распространяемые изображения. Лицензирование/разрешение на распространение таких изображений является ответственностью дистрибьютора конкретного map-pack.

## Collectible descriptions and location screenshots

Canonical descriptions, provenance and location screenshot references for the 320
collectibles are in [`data/collectibles/credits.json`](data/collectibles/credits.json).
Their licences are separate from the MIT licence of this program; see
[`data/collectibles/NOTICE.md`](data/collectibles/NOTICE.md).
