#!/usr/bin/env python3
"""Rebuild canonical collectible world/lookup data through the explicit crosswalk.

The existing collectibles.json is used only as a Wiki-derived metadata template:
descriptions, media paths, canonical IDs and localization are preserved. World
coordinates and runtime lookup fields are rebuilt from coordinate_catalog.json
through crosswalk.json; they are never taken from the template.
"""
import argparse
import copy
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACK = ROOT / "data" / "collectibles"
DEFAULT_DATA = PACK / "collectibles.json"
DEFAULT_CROSSWALK = PACK / "crosswalk.json"
DEFAULT_CATALOG = PACK / "coordinate_catalog.json"

EXPECTED = {
    "tags": 100,
    "snapshots": 50,
    "horseshoes": 50,
    "oysters": 50,
    "stunt_jumps": 70,
}
PRODUCTION_CONFIDENCE = {"confirmed", "high_confidence"}
WORLD_XY_LIMIT = 3000.0
WORLD_Z_MIN = -100.0
WORLD_Z_MAX = 1000.0
FLOAT_EPSILON = 1e-6


def load_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def close_world(a, b):
    return all(abs(float(a[k]) - float(b[k])) <= FLOAT_EPSILON for k in ("x", "y", "z"))


def validate_world(kind, coordinate_id, world):
    try:
        x, y, z = (float(world[k]) for k in ("x", "y", "z"))
    except (KeyError, TypeError, ValueError) as exc:
        raise ValueError(f"{kind} coordinate #{coordinate_id}: invalid XYZ") from exc
    if abs(x) > WORLD_XY_LIMIT or abs(y) > WORLD_XY_LIMIT:
        raise ValueError(f"{kind} coordinate #{coordinate_id}: outside GTA SA XY bounds: {world}")
    if not WORLD_Z_MIN <= z <= WORLD_Z_MAX:
        raise ValueError(f"{kind} coordinate #{coordinate_id}: suspicious Z: {z}")
    if x == 0.0 and y == 0.0 and z == 0.0:
        raise ValueError(f"{kind} coordinate #{coordinate_id}: suspicious default XYZ")
    if kind == "oysters" and z == 0.0:
        raise ValueError(f"oysters coordinate #{coordinate_id}: Z=0 is forbidden; use raw pickup XYZ")


def validate_crosswalk(crosswalk, catalog):
    if crosswalk.get("schema") != "gtasa-unexplored-collectibles-crosswalk-v1":
        raise ValueError("unexpected crosswalk schema")
    if catalog.get("schema") != "gtasa-unexplored-coordinate-catalog-v1":
        raise ValueError("unexpected coordinate catalog schema")

    cw_categories = crosswalk.get("categories", {})
    cat_categories = catalog.get("categories", {})
    indexes = {}

    if set(cw_categories) != set(EXPECTED) or set(cat_categories) != set(EXPECTED):
        raise ValueError("crosswalk/catalog category set differs from expected collectible types")

    for kind, expected in EXPECTED.items():
        cw_items = cw_categories[kind].get("items", [])
        raw_items = cat_categories[kind].get("items", [])
        if len(cw_items) != expected or len(raw_items) != expected:
            raise ValueError(f"{kind}: expected {expected} crosswalk/raw records")

        raw_by_id = {}
        for row in raw_items:
            cid = int(row["coordinate_catalog_id"])
            if cid in raw_by_id:
                raise ValueError(f"{kind}: duplicate coordinate_catalog_id {cid}")
            validate_world(kind, cid, row["world"])
            raw_by_id[cid] = row["world"]
        required = set(range(1, expected + 1))
        if set(raw_by_id) != required:
            raise ValueError(f"{kind}: raw coordinate IDs must be exactly 1..{expected}")

        cw_by_wiki = {}
        used_coord = set()
        used_save = set()
        for row in cw_items:
            wiki = int(row["wiki_ref"])
            cid = int(row["coordinate_catalog_id"])
            if wiki in cw_by_wiki:
                raise ValueError(f"{kind}: duplicate wiki_ref {wiki}")
            if cid in used_coord:
                raise ValueError(f"{kind}: duplicate mapped coordinate_catalog_id {cid}")
            if cid not in raw_by_id:
                raise ValueError(f"{kind} Wiki #{wiki}: coordinate_catalog_id {cid} missing")
            confidence = row.get("confidence")
            if confidence not in PRODUCTION_CONFIDENCE:
                raise ValueError(
                    f"{kind} Wiki #{wiki}: confidence {confidence!r} is not production-ready"
                )
            if not close_world(row.get("world", {}), raw_by_id[cid]):
                raise ValueError(
                    f"{kind} Wiki #{wiki}: crosswalk XYZ disagrees with raw coordinate #{cid}"
                )
            if kind == "tags":
                save_id = int(row["save_order_id"])
                if not 1 <= save_id <= 100 or save_id in used_save:
                    raise ValueError(f"tags Wiki #{wiki}: invalid/duplicate save_order_id {save_id}")
                used_save.add(save_id)
            cw_by_wiki[wiki] = row
            used_coord.add(cid)

        if set(cw_by_wiki) != required:
            raise ValueError(f"{kind}: Wiki refs must be exactly 1..{expected}")
        if used_coord != required:
            raise ValueError(f"{kind}: crosswalk coordinate IDs must be a bijection 1..{expected}")
        if kind == "tags" and used_save != set(range(1, 101)):
            raise ValueError("tags: save_order_id must be a bijection 1..100")
        indexes[kind] = (cw_by_wiki, raw_by_id)
    return indexes


def rebuild(template, crosswalk, catalog):
    if template.get("schema") != "gtasa-unexplored-collectibles-v1":
        raise ValueError("unexpected collectibles dataset schema")
    items = template.get("items", [])
    if len(items) != 320:
        raise ValueError("collectibles metadata template must contain exactly 320 items")

    indexes = validate_crosswalk(crosswalk, catalog)
    out = copy.deepcopy(template)
    seen = set()

    for item in out["items"]:
        kind = item.get("type")
        if kind not in EXPECTED:
            raise ValueError(f"unknown collectible type: {kind!r}")
        wiki = int(item.get("wiki_ref", item.get("canonical_id", 0)))
        key = (kind, wiki)
        if key in seen:
            raise ValueError(f"duplicate metadata item: {kind} Wiki #{wiki}")
        seen.add(key)

        cw_by_wiki, raw_by_id = indexes[kind]
        if wiki not in cw_by_wiki:
            raise ValueError(f"{kind}: Wiki #{wiki} absent from crosswalk")
        cw = cw_by_wiki[wiki]
        world = copy.deepcopy(raw_by_id[int(cw["coordinate_catalog_id"])])
        item["world"] = world

        if kind == "tags":
            save_id = int(cw["save_order_id"])
            item["lookup"] = {
                "strategy": "tag_save_order_id",
                "save_order_id": save_id,
                "source_index": save_id - 1,
            }
        else:
            item["lookup"] = {
                "strategy": "nearest_world_coordinate",
                "x": world["x"],
                "y": world["y"],
                "z": world["z"],
            }

    expected_keys = {
        (kind, wiki)
        for kind, count in EXPECTED.items()
        for wiki in range(1, count + 1)
    }
    if seen != expected_keys:
        raise ValueError("metadata template IDs are incomplete or out of range")
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=pathlib.Path, default=DEFAULT_DATA)
    parser.add_argument("--crosswalk", type=pathlib.Path, default=DEFAULT_CROSSWALK)
    parser.add_argument("--catalog", type=pathlib.Path, default=DEFAULT_CATALOG)
    parser.add_argument("--check", action="store_true",
                        help="Fail if canonical JSON is not exactly the explicit-crosswalk rebuild.")
    args = parser.parse_args()

    template = load_json(args.data)
    crosswalk = load_json(args.crosswalk)
    catalog = load_json(args.catalog)
    rebuilt = rebuild(template, crosswalk, catalog)

    rendered = json.dumps(rebuilt, ensure_ascii=False, indent=2) + "\n"
    if args.check:
        current = args.data.read_text(encoding="utf-8")
        # Accept legacy files without a final newline, but not semantic/data differences.
        if json.loads(current) != rebuilt:
            raise SystemExit("collectibles.json does not match explicit crosswalk/raw coordinate catalog")
        print("collectible crosswalk valid: 320 records")
        return

    args.data.write_text(rendered, encoding="utf-8", newline="\n")
    print("rebuilt collectibles.json through explicit crosswalk: 320 records")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, TypeError, ValueError, OSError, json.JSONDecodeError) as error:
        print("collectible crosswalk invalid:", error, file=sys.stderr)
        raise SystemExit(1)
