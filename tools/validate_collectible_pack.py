#!/usr/bin/env python3
"""Validate the distributable collectible pack using only the standard library."""
import hashlib
import json
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACK = ROOT / "data" / "collectibles"
EXPECTED = {"tags": 100, "snapshots": 50, "horseshoes": 50, "oysters": 50, "stunt_jumps": 70}


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


def main():
    data = json.loads((PACK / "collectibles.json").read_text(encoding="utf-8"))
    manifest = json.loads((PACK / "manifest.json").read_text(encoding="utf-8"))
    credits = json.loads((PACK / "credits.json").read_text(encoding="utf-8"))
    assert data["schema"] == "gtasa-unexplored-collectibles-v1"
    assert data["item_count"] == 320 and len(data["items"]) == 320
    assert manifest["item_count"] == manifest["image_count"] == manifest["expected_location_photo_count"] == 320
    assert credits["schema"] == "gtasa-unexplored-collectible-credits-v1"
    counts = {key: 0 for key in EXPECTED}
    seen = set()
    tag_orders = set()
    images = set()
    for item in data["items"]:
        kind, cid = item["type"], item["canonical_id"]
        assert kind in EXPECTED and 1 <= cid <= EXPECTED[kind] and (kind, cid) not in seen
        assert item["description_en"].strip() and item["description_ru"].strip()
        assert item["lookup"]["strategy"] in ("tag_save_order_id", "nearest_world_coordinate")
        if kind == "tags":
            tag_orders.add(item["lookup"]["save_order_id"])
        images.add(item["image"])
        counts[kind] += 1
    assert counts == EXPECTED and tag_orders == set(range(1, 101))
    records = {(entry["type"], entry["canonical_id"]): entry for entry in manifest["images"]}
    assert len(records) == 320 and len(images) == 320
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
    credit_items = credits.get("items", [])
    assert len(credit_items) == 320
    print("collectible pack valid: 320 records, 320 JPEG")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, KeyError, ValueError, OSError, json.JSONDecodeError) as error:
        print("collectible pack invalid:", error, file=sys.stderr)
        raise SystemExit(1)
