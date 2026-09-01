# San Andreas map pack

`maps.ini` describes five external map underlays in the application's `sa-world-v1` projection: X is `-3000…+3000` from left to right and Y is `+3000…-3000` from top to bottom. The game world consists of Los Santos, San Fierro, Las Venturas, and the five counties Red County, Flint County, Whetstone, Tierra Robada, and Bone County, which are represented across these maps. [GTA Wiki](https://gta.fandom.com/wiki/State_of_San_Andreas_(3D_Universe))

## Layout

- `source/` — user-supplied originals; never modified by the preparation script.
- `pack/` — generated square PNG underlays used by `maps.ini`; 2048×2048 is recommended for Switch.
- `../tools/prepare_map_pack.py` — reproducible converter; requires Pillow.

The official high-resolution source is a portrait promotional poster. The converter crops its square map field (`40,40` to `2540,2540`) before scaling; it deliberately excludes the legend, adverts, and logo. Geographic Regions is centre-cropped by one pixel at each vertical edge to correct its 1834×1836 input size. The other three maps are scaled without cropping.

The supported canvas range is **512…4096 px**; use `canvas_size` in `[pack]`.
For compatibility the loader also reads legacy `canvas`, but `canvas_size` wins
when both are present. A map image must remain square and match the declared
canvas size exactly.

Do a visual marker-alignment check on a Switch before relying on any underlay for gameplay: a same-size canvas alone cannot prove that a third-party map has the exact in-game projection. The PC helper `../tools/render_map_calibration.py` can write a diagnostic PNG with Los Santos, San Fierro, Las Venturas, airports, Area 69 and Mount Chiliad anchors. Keep only maps you are entitled to use and distribute.

## Deploy to Switch

Copy the complete `maps/` directory to:

```text
sdmc:/switch/gtasa-unexplored/maps/
```

The app reads `maps/maps.ini` and loads one texture at a time. If no usable map pack is present, it uses the embedded Apache-2.0 SVG fallback map instead.
