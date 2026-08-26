# Exact in-game colour map

GTASA Unexplored can use a user-supplied full-world map instead of the bundled
Apache-2.0 fallback.

## SD-card override

Put one square image at one of these paths (first match wins):

```text
/switch/gtasa-unexplored/map.png
/switch/gtasa-unexplored/map.jpg
/switch/gtasa-unexplored/map.webp
```

The image must show the **entire San Andreas world**, without UI borders or
cropping. Supported dimensions are 512..4096 pixels per side. 2048×2048 is the
recommended balance for Switch/app-mode memory use.

Coordinate mapping is fixed; no calibration is required:

```text
left   = world X -3000
right  = world X +3000
top    = world Y +3000
bottom = world Y -3000
```

Therefore marker positions remain correct at every pan/zoom level.

## Build the exact classic game radar from your own extracted tiles

The stock GTA San Andreas radar uses 144 images in a 12×12 grid. If you export
`radar00` through `radar143` from a game copy you own to PNG/WebP/JPEG/TGA, run:

```bash
python3 -m pip install Pillow
python3 tools/stitch_radar_tiles.py path/to/radar_tiles --output map.png
```

The tool validates all 144 tiles, places them in the game's row-major order and
resizes the atlas to 2048×2048 by default. Then copy `map.png` to:

```text
/switch/gtasa-unexplored/map.png
```

Use `--size 0` if you want to preserve the native atlas resolution, although
GTASA Unexplored intentionally rejects images above 4096×4096 to avoid excessive
memory use in Homebrew Menu applet mode.

The project **does not distribute Rockstar map/radar textures**. The stitching
script is only a transformation utility for assets supplied by the user.

## Bundled fallback

If no SD-card override exists, the release uses a colourized derivative of
Toliak's Apache-2.0 `San-Andreas-vector-map`, pinned to a known upstream commit.
Its colour palette is original to GTASA Unexplored and merely aims for the same
kind of readability as an in-game road/radar map; it is not Rockstar artwork.
