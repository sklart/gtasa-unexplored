# Collectible data model

GTASA Unexplored различает две задачи: получить **точный Missing** из save и восстановить координаты **Completed**.

## Tags

`TAGS` хранит 100 alpha-байтов. Индекс массива является внутренним save-order. `TagData.cpp` содержит координаты именно в этом порядке; он проверен по `gtasa-savegame-editor` `CollectablePageTags.addTags()`. Это намеренно не Wiki-нумерация.

Completed: `alpha > 228`.

## Stunt Jumps

`STUNTJUMPS` сам хранит все records и `done/found`, поэтому внешний каталог не нужен. Marker — центр XY start-box.

## Snapshots / Horseshoes / Oysters

После сбора one-shot pickup его активная запись исчезает/освобождается, поэтому из одного `PICKUPS` можно непосредственно получить только Missing. Для режима Completed/All используется нормализованный каталог 50+50+50 мировых координат в `CollectibleData.cpp`.

Активные save pickups сопоставляются с каталогом по X/Y в пределах небольшого допуска. Требования:

1. каждая активная запись должна иметь ровно одно подходящее место;
2. две записи не могут занять один canonical ID;
3. при любом конфликте категория помечается unreliable;
4. unreliable-категория сохраняет raw Missing (id=0), но не показывает выдуманные Completed coordinates.

Это позволяет пережить возможное изменение DE save layout/data без ложного результата.

### Oyster #27

В старом наборе `locations.toml` встречается `(2991,2991)`. Независимая map-выборка указывает `(2998,2998)`, и это значение использовано в каталоге. Тест фиксирует эту точку, чтобы расхождение не вернулось случайной правкой.

## Research references

Для проверки фактических координат и save semantics использовались публичные исследовательские проекты, в частности:

- `gtasa-savegame-editor/gtasa-savegame-editor` — save-order Tags;
- `user-grinch/GrinchTrainer-III-VC-SA` — исторический набор world coordinates для сверки;
- `interactive-game-maps/grand_theft_auto_san_andreas` — независимая сверка координат, включая Oyster #27;
- `gta-reversed/gta-reversed` — структура `CPickup`, flags и gameplay semantics.

В приложение не копируются их UI-код или игровые изображения; хранятся нормализованные фактические world coordinates, необходимые для сопоставления с save.
