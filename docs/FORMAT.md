# GTA San Andreas DE 1.1x save notes used by GTASA Unexplored

Scope: **GTA: San Andreas — The Definitive Edition**, current 1.1x save family introduced
with title update 1.112. The application is read-only and intentionally does not support
classic GTA SA or pre-1.112 DE saves.

## Container discovery

Published DE research describes named blocks as:

```text
uint32 name_length_including_nul
char   name[name_length_including_nul]
payload...
```

GTASA Unexplored therefore searches for NUL-terminated block names, prefers candidates
whose preceding DWORD exactly equals the name length, and accepts a payload only if its
internal structure validates. A small post-name wrapper search is retained as a diagnostic
fallback for 1.112 variations; fixed absolute file offsets are not used.

The three required payloads for v0.3 are:

- `PICKUPS`
- `TAGS`
- `STUNTJUMPS`

`diagnostics.txt` records every literal candidate and whether it had the documented length
prefix, which is useful if a real Switch save uses an unseen wrapper.

## Header diagnostics

The parser reports, without modifying the save:

- first 4 bytes (magic/signature) as hex;
- DWORD at `0x04`;
- stored 16-byte checksum field at `0x08`;
- DWORD at `0x18`;
- fields at `0x1C` and `0x20`;
- the length at `0x24` and printable last-mission GXT key that follows it.

The v0.3 Switch application does **not** need to recalculate or write the checksum because it
never writes to the game save. Public research says 1.112 applies one's-complement to the
MD5 value after hashing a copy with the first 24 bytes zeroed; the earlier Python research
prototype can be used if checksum verification is later desired.

## PICKUPS

For the fields used here the save payload is treated as 620 records of `0x20` bytes.
Coordinates are compressed signed 16-bit values at offsets `0x10/0x12/0x14`, divided by 8.
Model ID is `uint16` at `0x18`; pickup type is `uint8` at `0x1C`.

Remaining collectible records:

| Collectible | Model | Pickup type |
|---|---:|---:|
| Oyster | 953 | `PICKUP_ONCE` = 3 |
| Horseshoe | 954 | `PICKUP_ONCE` = 3 |
| Snapshot | 1253 | `PICKUP_SNAPSHOT` = 20 |

Collected records are no longer active collectible pickup records, so the active records
form the missing-object list. Counts are inferred as `50 - missing`.

## TAGS

Payload:

```text
uint32 count        // expected 100 for vanilla SA
uint8  alpha[count]
```

The original game logic defines `ALPHA_TAGGED = 228` and counts a tag as complete when
`alpha > 228`, i.e. 229..255. The 100 alpha bytes contain no coordinates; world coordinates
are taken from the published save-order table in the MIT-licensed GTA SA Savegame Editor.

## STUNTJUMPS

Payload starts with a count followed by `0x44`-byte records. Fields used by the app:

- start zone point 1: floats at `+0x00 .. +0x08`;
- start zone point 2: floats at `+0x0C .. +0x14`;
- reward: `uint32` at `+0x3C` (diagnostic only at present);
- completed/done: byte `+0x40`;
- found/triggered: byte `+0x41`.

A jump is missing when `done == 0`. The map marker is placed at the midpoint of the two
start-zone XY points. `found == 1 && done == 0` is shown as "discovered but not completed".

## Map coordinates

Common GTA SA map tooling treats the map as a 6000-unit square with world `(0,0)` in the
center. For a square texture:

```text
u = (x + 3000) / 6000
v = (3000 - y) / 6000
```

The build uses the Apache-2.0 `Toliak/San-Andreas-vector-map` SVG, whose canvas is
6000×6000, rasterized to a square PNG. Marker projection uses the same transform for all
zoom and pan operations.

## Validation status

Completed locally:

- positive synthetic parser regression;
- decoy-name precedence test (documented length-prefixed block wins);
- random/invalid/truncated negative parser tests;
- map world↔texture math regression;
- host build of the parser inspector and non-Switch platform layer with
  `-Wall -Wextra -Werror`.

Still requires external hardware/toolchain validation:

- devkitA64 cross-link of the complete NRO;
- one real DE 1.1x save binary through the parser;
- visual alignment check of the rasterized vector map on Switch;
- physical Switch launch/profile/save mount test.
