#!/usr/bin/env python3
"""Apply the conservative POI coordinate policy and write its audit trail.

Only the two object points independently evidenced by the existing collectible
catalogue are emitted as XYZ.  Area/route markers deliberately have no Z.
Everything else remains an honest pending record.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / "data" / "poi" / "poi.json"
AUDIT = ROOT / "data" / "poi" / "coordinate_audit.json"

# x/y are deliberately presentation markers, not teleport positions.  They
# identify a broad feature and are not used for concrete buildings.
REPRESENTATIVE = {
    10: (360.0, -1929.0, "beach area", "Santa Maria Beach"),
    41: (2423.8, 2107.5, "casino district", "Old Venturas Strip"),
    48: (-286.9, 2170.3, "ghost-town area", "Las Brujas"),
    53: (257.6, 1413.1, "settlement area", "Green Palms"),
    54: (-694.6, 1794.5, "reservoir water area", "Sherman Reservoir"),
    56: (-244.0, 2698.6, "settlement area", "Las Payasadas"),
    59: (-2547.1, 2449.0, "town area", "Bayside"),
    61: (-1324.9, 2556.9, "settlement area", "Aldea Malvada"),
    63: (-822.7, 1516.7, "town area", "Las Barrancas"),
    64: (-826.3, 2729.7, "town area", "Valle Ocultado"),
    67: (-107.6, 36.6, "agricultural district", "Blueberry Acres"),
    70: (907.4, -9.9, "settlement area", "Fern Ridge"),
    74: (-744.4, -2248.6, "backcountry area", "Back o Beyond"),
    75: (-744.4, -2248.6, "lake area", "Back o Beyond"),
    76: (-2057.4, -1720.4, "mountain area", "Mount Chiliad"),
    77: (1249.0, -2687.0, "route marker", None),
    78: (-1925.4, -1973.3, "rural area", "Shady Creeks"),
    79: (-2144.6, -2398.2, "town area", "Angel Pine"),
}

# These are object-specific points, not inferred centres.  Each is cross-checked
# against the canonical local collectibles catalogue and a named info.zon area.
CROSS_CHECKED = {
    28: (-1906.66, 518.58, 61.71, "Downtown, San Fierro", "Snapshot #50"),
    37: (1224.0, 2617.0, 11.0, "Las Venturas", "Horseshoe #1"),
}


def main():
    data = json.loads(PACK.read_text(encoding="utf-8"))
    audit = []
    for item in data["items"]:
        poi_id = item["id"]
        item.pop("coordinate_kind", None)
        item.pop("coordinate_accuracy_note", None)
        item.pop("coordinate_sources", None)
        item.pop("coordinate_confidence", None)
        item.pop("info_zon_check", None)
        if poi_id in CROSS_CHECKED:
            x, y, z, zone, catalogue = CROSS_CHECKED[poi_id]
            item.update({
                "world": {"x": x, "y": y, "z": z},
                "coordinate_kind": "feature_xyz",
                "coordinate_status": "cross_checked_xyz",
                "coordinate_confidence": "high",
                "coordinate_sources": [
                    {"kind": "local_collectible_catalogue", "reference": catalogue,
                     "note": "Independent authored point whose description names this POI."},
                    {"kind": "info_zon", "reference": zone,
                     "note": "Spatial membership check only; not a coordinate source."},
                ],
                "info_zon_check": {"checked": True, "expected_zone": zone},
            })
        elif poi_id in REPRESENTATIVE:
            x, y, feature, zone = REPRESENTATIVE[poi_id]
            item.update({
                "world": {"x": x, "y": y, "z": None},
                "coordinate_kind": "representative_2d",
                "coordinate_status": "representative_2d",
                "coordinate_confidence": "medium",
                "coordinate_accuracy_note": "Map marker for a %s, not a teleport coordinate." % feature,
                "coordinate_sources": [
                    {"kind": "project_curated_map_marker_v1", "reference": item["source_page_url_en"],
                     "note": "Curated presentation marker for an extended POI; it is explicitly not XYZ."},
                ],
                "info_zon_check": {"checked": zone is not None, "expected_zone": zone},
            })
        else:
            item.update({
                "world": None,
                "coordinate_kind": "none",
                "coordinate_status": "pending_verification",
                "coordinate_confidence": "none",
                "coordinate_sources": [],
                "info_zon_check": {"checked": False},
            })
        audit.append({
            "id": poi_id, "name_en": item["name_en"], "region": item["region"],
            "world": item["world"], "coordinate_kind": item["coordinate_kind"],
            "coordinate_status": item["coordinate_status"],
            "confidence": item["coordinate_confidence"],
            "sources": item["coordinate_sources"], "info_zon_check": item["info_zon_check"],
            **({"accuracy_note": item["coordinate_accuracy_note"]}
               if "coordinate_accuracy_note" in item else {}),
        })
    PACK.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    AUDIT.write_text(json.dumps({"schema": "gtasa-unexplored-poi-coordinate-audit-v1",
                                 "policy": "conservative-v2", "items": audit},
                                ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
