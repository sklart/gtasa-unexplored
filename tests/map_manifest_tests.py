#!/usr/bin/env python3
"""Lightweight manifest compatibility checks without requiring image assets."""
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
VALIDATOR = ROOT / "tools" / "validate_map_pack.py"


def check(pack_fields, expected_canvas):
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        (root / "maps.ini").write_text(
            "[pack]\nformat=1\nprojection=sa-world-v1\n" + pack_fields + "\n"
            "[map]\nkind=overlay\nid=test\nfile=missing.png\n", encoding="utf-8")
        completed = subprocess.run([sys.executable, str(VALIDATOR), str(root),], text=True,
                                   capture_output=True, check=False)
        if completed.returncode != 0:
            raise AssertionError(completed.stderr or completed.stdout)
        if f"canvas_size={expected_canvas}" not in completed.stdout:
            raise AssertionError(completed.stdout)


check("canvas_size=2048", 2048)
check("canvas=2048", 2048)
check("canvas=1024\ncanvas_size=2048", 2048)
print("map manifest compatibility tests passed")
