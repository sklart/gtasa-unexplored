#!/usr/bin/env python3
"""Validate collectible identities, canonical data and (optionally) distributable media."""
import argparse
import hashlib
import json
import pathlib
import struct
import sys

from apply_collectible_crosswalk import load_json, rebuild, validate_crosswalk

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACK = ROOT / "data" / "collectibles"
EXPECTED = {
    "tags": 100,
    "snapshots": 50,
    "horseshoes": 50,
    "oysters": 50,
    "stunt_jumps": 70,
}


def jpeg_size(path):
    data = path.read_bytes()
    if not data.startswith(b"\xff\xd8"):
        raise ValueError("not a JPEG")
    offset = 2
    while offset + 9 < len(data):
        if data[offset] != 0xff:
            raise ValueError("invalid JPEG marker")
        while offset < len(data) and data[offset] == 0xff:
            offset += 1
        marker = data[offset]
        offset += 1
        if marker in (0xd8, 0xd9) or 0xd0 <= marker <= 0xd7:
            continue
        length = struct.unpack(">H", data[offset:offset + 2])[0]
        if length < 2 or offset + length > len(data):
            raise ValueError("truncated JPEG")
        if marker in (0xc0, 0xc1, 0xc2, 0xc3, 0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf):
            height, width = struct.unpack(">HH", data[offset + 3:offset + 7])
            return width, height
        offset += length
    raise ValueError("JPEG SOF marker missing")


def relative(path):
    candidate = pathlib.PurePosixPath(path)
    if candidate.is_absolute() or ".." in candidate.parts or candidate.suffix.lower() not in (".jpg", ".jpeg"):
        raise ValueError("unsafe image path: " + path)
    return PACK.joinpath(*candidate.parts)


def validate_identity_data(data, crosswalk, catalog):
    validate_crosswalk(crosswalk, catalog)
    rebuilt = rebuild(data, crosswalk, catalog)
    if data != rebuilt:
        raise ValueError(
            "collectibles.json is not the canonical explicit-crosswalk rebuild; "
            "run tools/apply_collectible_crosswalk.py"
        )

    assert data["schema"] == "gtasa-unexplored-collectibles-v1"
    assert data["item_count"] == 320 and len(data["items"]) == 320
    counts = {key: 0 for key in EXPECTED}
    seen = set()
    tag_orders = set()
    images = set()
    for item in data["items"]:
        kind, cid = item["type"], int(item["canonical_id"])
        wiki = int(item["wiki_ref"])
        assert kind in EXPECTED and 1 <= cid <= EXPECTED[kind]
        assert cid == wiki, "canonical_id remains the Wiki identity"
        assert (kind, wiki) not in seen
        seen.add((kind, wiki))
        assert item["description_en"].strip() and item["description_ru"].strip()
        world = item["world"]
        assert item["lookup"]["strategy"] in ("tag_save_order_id", "nearest_world_coordinate")
        if kind == "tags":
            save_id = int(item["lookup"]["save_order_id"])
            assert int(item["lookup"]["source_index"]) == save_id - 1
            tag_orders.add(save_id)
        else:
            assert float(item["lookup"]["x"]) == float(world["x"])
            assert float(item["lookup"]["y"]) == float(world["y"])
            assert float(item["lookup"]["z"]) == float(world["z"])
        if kind == "oysters":
            assert float(world["z"]) != 0.0, "Oyster Z must come from raw pickup XYZ"
        images.add(item["image"])
        counts[kind] += 1
    assert counts == EXPECTED and tag_orders == set(range(1, 101))
    assert len(images) == 320
    return images


def validate_media(data, images):
    manifest = json.loads((PACK / "manifest.json").read_text(encoding="utf-8"))
    credits = json.loads((PACK / "credits.json").read_text(encoding="utf-8"))
    assert manifest["item_count"] == manifest["image_count"] == manifest["expected_location_photo_count"] == 320
    assert credits["schema"] == "gtasa-unexplored-collectible-credits-v1"
    records = {(entry["type"], entry["canonical_id"]): entry for entry in manifest["images"]}
    assert len(records) == 320
    for item in data["items"]:
        record = records[(item["type"], item["canonical_id"])]
        assert record["path"] == item["image"]
        path = relative(record["path"])
        assert path.is_file() and path.stat().st_size == record["bytes"]
        assert hashlib.sha256(path.read_bytes()).hexdigest() == record["sha256"]
        width, height = jpeg_size(path)
        assert width == record["width"] and height == record["height"] and width <= 960 and height <= 540
    files = {p.relative_to(PACK).as_posix() for p in (PACK / "images").rglob("*.jpg")}
    assert files == images
    assert len(credits.get("items", [])) == 320


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--metadata-only",
        action="store_true",
        help="Validate all 320 identities/coordinates/lookups but skip JPEG/manifest byte checks.",
    )
    args = parser.parse_args()

    data = load_json(PACK / "collectibles.json")
    crosswalk = load_json(PACK / "crosswalk.json")
    catalog = load_json(PACK / "coordinate_catalog.json")
    images = validate_identity_data(data, crosswalk, catalog)
    if not args.metadata_only:
        validate_media(data, images)
        print("collectible pack valid: 320 identities, crosswalk, runtime lookup and 320 JPEG")
    else:
        print("collectible metadata valid: 320 identities, crosswalk and runtime lookup")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, KeyError, TypeError, ValueError, OSError, json.JSONDecodeError) as error:
        print("collectible pack invalid:", error, file=sys.stderr)
        raise SystemExit(1)
