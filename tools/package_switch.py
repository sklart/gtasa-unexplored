#!/usr/bin/env python3
"""Create the SD-root install package without putting media in the NRO."""
import pathlib
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
NRO = ROOT / "gtasa-unexplored.nro"
PACK = ROOT / "data" / "collectibles"
OUT = ROOT / "gtasa-unexplored-switch.zip"

with zipfile.ZipFile(OUT, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    archive.write(NRO, "switch/gtasa-unexplored/gtasa-unexplored.nro")
    for path in PACK.rglob("*"):
        if path.is_file():
            archive.write(path, "switch/gtasa-unexplored/collectibles/" + path.relative_to(PACK).as_posix())
print(OUT)
