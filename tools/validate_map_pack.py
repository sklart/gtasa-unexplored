#!/usr/bin/env python3
"""Validate a GTASA Unexplored map-pack v1 manifest and its raster dimensions."""
import configparser
import pathlib
import sys


def main(argv):
    if len(argv) != 2:
        raise SystemExit(f"usage: {argv[0]} <maps-directory>")
    root = pathlib.Path(argv[1])
    manifest = root / "maps.ini"
    parser = configparser.ConfigParser(strict=False)
    if not parser.read(manifest, encoding="utf-8") or "pack" not in parser:
        raise SystemExit("maps.ini with [pack] is required")
    pack = parser["pack"]
    if pack.get("format") != "1" or pack.get("projection") != "sa-world-v1":
        raise SystemExit("expected format=1 and projection=sa-world-v1")
    canvas = pack.getint("canvas_size", fallback=pack.getint("canvas", fallback=0))
    if not 512 <= canvas <= 4096:
        raise SystemExit("canvas_size/canvas must be 512..4096")
    map_sections = [section for section in parser.sections() if section.lower().startswith("map")]
    if not map_sections:
        raise SystemExit("at least one [map] section is required")
    try:
        from PIL import Image
    except ImportError:
        print(f"manifest valid: canvas_size={canvas}; Pillow unavailable, skipped raster checks")
        return
    for section in map_sections:
        entry = parser[section]
        if entry.get("kind", "base").lower() != "base":
            continue
        image = root / entry.get("file", "")
        if not image.is_file():
            raise SystemExit(f"missing map image: {image}")
        with Image.open(image) as raster:
            if raster.size != (canvas, canvas):
                raise SystemExit(f"{image}: expected {canvas}x{canvas}, got {raster.size[0]}x{raster.size[1]}")
    print(f"map pack valid: canvas_size={canvas}")


if __name__ == "__main__":
    main(sys.argv)
