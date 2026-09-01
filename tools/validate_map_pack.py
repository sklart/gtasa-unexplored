#!/usr/bin/env python3
"""Validate a GTASA Unexplored map-pack v1 manifest and its raster dimensions."""
import pathlib
import struct
import sys


def parse_manifest(path):
    pack, maps, current = {}, [], None
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        if line.startswith("[") and line.endswith("]"):
            name = line[1:-1].strip().lower()
            if name == "pack": current = pack
            elif name == "map": current = {}; maps.append(current)
            else: current = None
            continue
        if current is not None and "=" in line:
            key, value = line.split("=", 1)
            current[key.strip().lower()] = value.strip()
    return pack, maps


def canvas_size(pack):
    try:
        value = int(pack.get("canvas_size", pack.get("canvas", "")))
    except ValueError as error:
        raise ValueError("canvas_size/canvas must be an integer") from error
    if not 512 <= value <= 4096:
        raise ValueError("canvas_size/canvas must be 512..4096")
    return value


def validate(root, check_rasters=True):
    manifest = root / "maps.ini"
    if not manifest.is_file(): raise ValueError("maps.ini with [pack] is required")
    pack, maps = parse_manifest(manifest)
    if pack.get("format") != "1" or pack.get("projection") != "sa-world-v1":
        raise ValueError("expected format=1 and projection=sa-world-v1")
    canvas = canvas_size(pack)
    base_maps = [entry for entry in maps if entry.get("kind", "base").lower() == "base"]
    if not base_maps: raise ValueError("at least one base [map] entry is required")
    if not check_rasters: return canvas, len(base_maps)
    for index, entry in enumerate(base_maps, 1):
        image = root / entry.get("file", "")
        if not image.is_file():
            raise ValueError(f"map #{index}: missing raster {image}")
        if image.suffix.lower() == ".png":
            header = image.read_bytes()[:24]
            if header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
                raise ValueError(f"map #{index}: invalid PNG header {image}")
            size = struct.unpack(">II", header[16:24])
        else:
            try:
                from PIL import Image
            except ImportError as error:
                raise ValueError(f"map #{index}: Pillow is required to validate {image.suffix} raster dimensions") from error
            with Image.open(image) as raster:
                size = raster.size
        if size != (canvas, canvas):
            raise ValueError(f"map #{index}: expected {canvas}x{canvas}, got {size[0]}x{size[1]}")
    return canvas, len(base_maps)


def main(argv):
    if len(argv) != 2: raise SystemExit(f"usage: {argv[0]} <maps-directory>")
    try:
        canvas, count = validate(pathlib.Path(argv[1]))
    except ValueError as error:
        raise SystemExit(error)
    print(f"map pack valid: canvas_size={canvas}; base maps={count}")


if __name__ == "__main__":
    main(sys.argv)
