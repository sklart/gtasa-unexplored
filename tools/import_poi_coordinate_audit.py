#!/usr/bin/env python3
"""Synchronize canonical POI records from an independently reviewed audit."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / "data" / "poi" / "poi.json"
AUDIT = ROOT / "data" / "poi" / "coordinate_audit.json"
VISIBLE = {"cross_checked_xyz", "representative_2d"}


def fail(message):
    raise SystemExit("POI audit import refused: " + message)


def main():
    pack = json.loads(PACK.read_text(encoding="utf-8"))
    audit = json.loads(AUDIT.read_text(encoding="utf-8"))
    if audit.get("schema") != "gtasa-unexplored-poi-coordinate-audit-v1":
        fail("unexpected audit schema")
    if len(pack.get("items", [])) != 80 or len(audit.get("items", [])) != 80:
        fail("both pack and audit must contain 80 records")
    by_id = {item["id"]: item for item in audit["items"]}
    if len(by_id) != 80:
        fail("audit IDs are not unique")
    for item in pack["items"]:
        source = by_id.get(item["id"])
        if not source or source.get("name_en") != item.get("name_en"):
            fail("ID/name mismatch for %r" % item.get("id"))
        status, world = source.get("coordinate_status"), source.get("world")
        if status == "cross_checked_xyz":
            if not isinstance(world, dict) or world.get("z") is None:
                fail("cross_checked_xyz needs XYZ for %s" % item["id"])
        elif status == "representative_2d":
            if not isinstance(world, dict) or world.get("z") is not None:
                fail("representative_2d needs XY/null-Z for %s" % item["id"])
        elif status == "pending_verification":
            if world is not None:
                fail("pending_verification must not have coordinates for %s" % item["id"])
        else:
            fail("unknown status for %s" % item["id"])
        for key in ("world", "coordinate_kind", "coordinate_status", "coordinate_confidence",
                    "coordinate_sources", "coordinate_accuracy_note", "info_zon_check",
                    "coordinate_note"):
            item.pop(key, None)
        item["world"] = world
        item["coordinate_kind"] = source.get("coordinate_kind", "none")
        item["coordinate_status"] = status
        item["coordinate_confidence"] = source.get("confidence", "none")
        item["coordinate_sources"] = source.get("sources", [])
        item["info_zon_check"] = source.get("info_zon_check", {"checked": False})
        if "coordinate_accuracy_note" in source:
            item["coordinate_accuracy_note"] = source["coordinate_accuracy_note"]
        if "audit_note" in source:
            item["coordinate_note"] = source["audit_note"]
    PACK.write_text(json.dumps(pack, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
