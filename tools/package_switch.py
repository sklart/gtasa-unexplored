#!/usr/bin/env python3
"""Create the SD-root install package without putting media in the NRO."""
import pathlib
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
NRO = ROOT / "gtasa-unexplored.nro"
PACKS = ("collectibles", "poi")
OUT = ROOT / "gtasa-unexplored-switch.zip"

with zipfile.ZipFile(OUT, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    archive.write(NRO, "switch/gtasa-unexplored/gtasa-unexplored.nro")
    for pack_name in PACKS:
        pack = ROOT / "data" / pack_name
        for path in pack.rglob("*"):
            if path.is_file():
                archive.write(path, "switch/gtasa-unexplored/" + pack_name + "/" + path.relative_to(pack).as_posix())
print(OUT)
