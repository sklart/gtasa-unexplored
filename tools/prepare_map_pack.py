"""Build the external GTASA Unexplored v1 map pack from user-supplied sources.

The input files stay untouched in maps/source.  Output is deterministic PNG
at 2048x2048, the canvas required by maps/maps.ini.
"""

from pathlib import Path
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "maps" / "source"
OUTPUT = ROOT / "maps" / "pack"
CANVAS = 2048

# The official Rockstar image is a portrait poster: its square map field is
# x=40..2540, y=40..2540; the legend and adverts below it are intentionally
# outside this crop.  Other maps contain the complete world in their canvas.
SPECS = {
    "terrain.png": ("SanAndreas-TerrainMap.webp", None),
    "official-high-res.png": ("SanAndreasState-GTASA-OfficialRockstarHighResDownload.webp", (40, 40, 2540, 2540)),
    "street.png": ("Sanandreas_map.webp", None),
    "definitive-edition.png": ("SanAndreas-GTASAde-Map.webp", None),
    "geographic-regions.png": ("GeoRegions-GTASA.webp", (0, 1, 1834, 1835)),
}


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for output_name, (input_name, crop) in SPECS.items():
        with Image.open(SOURCE / input_name) as image:
            image = image.convert("RGB")
            if crop:
                image = image.crop(crop)
            if image.width != image.height:
                raise ValueError(f"{input_name}: expected square input after crop, got {image.size}")
            image = image.resize((CANVAS, CANVAS), Image.Resampling.LANCZOS)
            image.save(OUTPUT / output_name, "PNG", optimize=True, compress_level=9)
            print(f"{output_name}: {image.size[0]}x{image.size[1]}")


if __name__ == "__main__":
    main()
