#!/usr/bin/env python3
"""Render known SA world-coordinate anchors over a map image for alignment review."""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw

ANCHORS = {
    "Los Santos": (1481, -1772), "San Fierro": (-1985, 138),
    "Las Venturas": (1698, 1448), "Mount Chiliad": (-2324, -1625),
    "LS Airport": (1685, -2334), "SF Airport": (-1336, -268),
    "LV Airport": (1697, 1618), "Area 69": (276, 2023),
}

parser = argparse.ArgumentParser()
parser.add_argument("image", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--world-left", type=float, default=-3000)
parser.add_argument("--world-right", type=float, default=3000)
parser.add_argument("--world-top", type=float, default=3000)
parser.add_argument("--world-bottom", type=float, default=-3000)
args = parser.parse_args()

image = Image.open(args.image).convert("RGBA")
draw = ImageDraw.Draw(image)
for name, (x, y) in ANCHORS.items():
    px = (x - args.world_left) * image.width / (args.world_right - args.world_left)
    py = (args.world_top - y) * image.height / (args.world_top - args.world_bottom)
    draw.ellipse((px - 6, py - 6, px + 6, py + 6), fill=(255, 70, 60, 255), outline=(0, 0, 0, 255), width=2)
    draw.text((px + 8, py - 6), name, fill=(255, 255, 255, 255), stroke_width=2, stroke_fill=(0, 0, 0, 255))
image.save(args.output)
print(args.output)
