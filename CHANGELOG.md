# Changelog

## 0.7.0 RC

- Добавлена полная модель 320 объектов и режимы `Missing / Completed / All`.
- Добавлены канонические каталоги 50 Snapshots, 50 Horseshoes и 50 Oysters со stable ID.
- Для pickup-категорий реализовано fail-closed сопоставление активных `PICKUPS` с каталогом по координатам: при неоднозначности Completed не реконструируется, точный Missing сохраняется.
- Completed-маркеры в `All` визуально приглушены; UI предупреждает о неполных координатах Completed.
- Подтверждён и защищён regression-тестом настоящий save-array order всех 100 Tags.
- Добавлен расширенный `inspect-save --json` / `--objects`.
- Switch export по ↓ теперь создаёт также `saveinfo.json` schema=1; сериализатор общий с `inspect-save --json`.
- Временные raw-pickup marks с `id=0` привязаны к координатам, а не к переиспользуемому PICKUPS slot.
- При успешном live-read очищаются stale temporary marks всех обнаруженных GTA slots.
- `STUNTJUMPS` validator для текущего target ужесточён до ровно 70 records.
- Negative parser corpus расширен deterministic mutations/injected block names и проходит ASan/UBSan.
- Map-pack format 1 зафиксирован на `canvas=2048`; runtime и PC validator отклоняют другой размер.
- Map-pack tests теперь проверяют реальные 1024×1024/2048×2048 изображения.
- Усилены parser/data regressions: ровно 100/50/50/50/70 объектов и согласованность completed-counts.
- Временные backup-отметки работают со stable ID и остаются раздельными по профилю/слоту.
- Версия приложения и Makefile обновлены до 0.7.0.

## 0.6.0 RC

- Внешние versioned map packs (`format=1`, `projection=sa-world-v1`).
- ZL/ZR switching без изменения center/zoom; Y hot reload.
- Удалены встроенные map-assets и ROMFS dependency.
- GTA save mount физически ограничен read-only API.
- Slot-specific temporary marks для stale backup.
- Добавлена DE 1.112 complemented-MD5 checksum verification и raw-MD5 detection.
- Исправлена семантика pickup flags: только `bDisabled` bit 0 исключает matching pickup.
- Добавлены regression/negative/MD5/source-consistency/Switch-syntax/ASan/UBSan tests.
