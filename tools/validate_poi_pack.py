#!/usr/bin/env python3
"""Validate POI coordinate-policy invariants and report their exact counts."""
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / "data" / "poi" / "poi.json").read_text(encoding="utf-8"))
audit = json.loads((ROOT / "data" / "poi" / "coordinate_audit.json").read_text(encoding="utf-8"))
items = data.get("items", [])
if len(items) != 80 or len(audit.get("items", [])) != 80:
    raise SystemExit("POI pack or audit must contain exactly 80 records")
audit_by_id = {item["id"]: item for item in audit["items"]}
if len(audit_by_id) != 80:
    raise SystemExit("POI audit IDs must be unique")
counts = Counter()
for item in items:
    status, world = item.get("coordinate_status"), item.get("world")
    source = audit_by_id.get(item["id"])
    if not source or source.get("name_en") != item.get("name_en"):
        raise SystemExit("POI/audit ID or English-name mismatch: %s" % item.get("id"))
    if source.get("coordinate_status") != status or source.get("world") != world:
        raise SystemExit("POI/audit coordinate mismatch: %s" % item["id"])
    counts[status] += 1
    if status == "cross_checked_xyz":
        if not world or world.get("z") is None or not item.get("coordinate_sources") or not item.get("info_zon_check", {}).get("checked"):
            raise SystemExit("invalid cross_checked_xyz: %s" % item["id"])
    elif status == "representative_2d":
        if not world or world.get("z") is not None or not item.get("coordinate_accuracy_note") or not item.get("coordinate_sources"):
            raise SystemExit("invalid representative_2d: %s" % item["id"])
    elif status == "pending_verification":
        if world is not None:
            raise SystemExit("pending POI must not have a map point: %s" % item["id"])
    else:
        raise SystemExit("unknown coordinate status: %r" % status)
print("cross_checked_xyz=%d representative_2d=%d pending_verification=%d" %
      (counts["cross_checked_xyz"], counts["representative_2d"], counts["pending_verification"]))
if (counts["cross_checked_xyz"], counts["representative_2d"], counts["pending_verification"]) != (21, 18, 41):
    raise SystemExit("unexpected coordinate-status totals")
