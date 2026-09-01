#!/usr/bin/env python3
"""Map manifest compatibility and repeated-[map] parser regression tests."""
import importlib.util
import pathlib
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("map_validator", ROOT / "tools" / "validate_map_pack.py")
validator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validator)


def check(pack_fields, expected_canvas):
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        (root / "maps.ini").write_text(
            "[pack]\nformat=1\nprojection=sa-world-v1\n" + pack_fields + "\n"
            "[map]\nkind=base\nid=first\nfile=first.png\n"
            "[map]\nkind=base\nid=second\nfile=second.png\n"
            "[map]\nkind=base\nid=third\nfile=third.png\n", encoding="utf-8")
        for name in ("first.png", "second.png", "third.png"):
            (root / name).write_bytes(b"\x89PNG\r\n\x1a\n" + b"\x00\x00\x00\rIHDR" +
                                      expected_canvas.to_bytes(4, "big") + expected_canvas.to_bytes(4, "big"))
        canvas, count = validator.validate(root)
        assert canvas == expected_canvas
        assert count == 3


check("canvas_size=2048", 2048)
check("canvas=2048", 2048)
check("canvas=1024\ncanvas_size=2048", 2048)
print("map manifest compatibility tests passed")
