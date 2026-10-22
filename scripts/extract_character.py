"""Cuts the character (guy + office chair) out of the original mockup.

    python3 scripts/extract_character.py            -> Assets/character.png

The mockup's cream background is flood-filled from the crop border; everything the
cartoon's black outline encloses is kept, so the artwork itself is never redrawn.
Stray marks (ruler, arrow, knob ticks) are dropped by keeping the largest shape.
"""
import sys
from collections import deque
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "Assets" / "Layout-mockup.png"
DST = ROOT / "Assets" / "character.png"

CROP = (800, 210, 1310, 840)   # x0, y0, x1, y1 in the 1536x1024 mockup
PAD_TOP = 70                   # room above the head for the face bulge

def main():
    rgb = np.asarray(Image.open(SRC).convert("RGB").crop(CROP)).astype(np.float32)
    h, w, _ = rgb.shape
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    lum = 0.299 * r + 0.587 * g + 0.114 * b
    sat = rgb.max(axis=2) - rgb.min(axis=2)

    # Paper: light and only slightly warm. Below the knees the floor shadow is darker paper.
    ys = np.arange(h)[:, None]
    paper = (lum > 175) & (sat < 70)
    shadow = (ys > 380) & (lum > 135) & (sat < 85) & (r >= g) & (g >= b)
    candidate = paper | shadow

    background = np.zeros((h, w), bool)
    queue = deque()
    for x in range(w):
        for y in (0, h - 1):
            if candidate[y, x] and not background[y, x]:
                background[y, x] = True; queue.append((y, x))
    for y in range(h):
        for x in (0, w - 1):
            if candidate[y, x] and not background[y, x]:
                background[y, x] = True; queue.append((y, x))
    while queue:
        y, x = queue.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ny, nx = y + dy, x + dx
            if 0 <= ny < h and 0 <= nx < w and candidate[ny, nx] and not background[ny, nx]:
                background[ny, nx] = True
                queue.append((ny, nx))

    # Paper seen through the gaps between the legs and the chair is enclosed by outlines,
    # so the flood fill can't reach it: clear those pockets too (shoe soles are lower).
    pocket = np.zeros((h, w), bool)
    pocket[380:540, 150:340] = True
    seen = background.copy()
    for sy in range(380, 540):
        for sx in range(150, 340):
            if paper[sy, sx] and not seen[sy, sx]:
                region = [(sy, sx)]; seen[sy, sx] = True; queue.append((sy, sx)); inside = True
                while queue:
                    y, x = queue.popleft()
                    inside &= bool(pocket[y, x])
                    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        ny, nx = y + dy, x + dx
                        if 0 <= ny < h and 0 <= nx < w and paper[ny, nx] and not seen[ny, nx]:
                            seen[ny, nx] = True; queue.append((ny, nx)); region.append((ny, nx))
                if inside and len(region) > 150:
                    for y, x in region:
                        background[y, x] = True

    # Keep the largest foreground shape (the character).
    fg = ~background
    label = np.zeros((h, w), np.int32)
    sizes = [0]
    for sy in range(h):
        for sx in range(w):
            if fg[sy, sx] and label[sy, sx] == 0:
                idx = len(sizes); count = 0
                label[sy, sx] = idx; queue.append((sy, sx))
                while queue:
                    y, x = queue.popleft(); count += 1
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            ny, nx = y + dy, x + dx
                            if 0 <= ny < h and 0 <= nx < w and fg[ny, nx] and label[ny, nx] == 0:
                                label[ny, nx] = idx
                                queue.append((ny, nx))
                sizes.append(count)
    keep = label == int(np.argmax(sizes))

    # Soft 1 px edge: average the mask over 3x3, but never fade the inside.
    padded = np.pad(keep.astype(np.float32), 1)
    soft = sum(padded[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] for dy in (-1, 0, 1) for dx in (-1, 0, 1)) / 9.0
    alpha = np.where(keep, np.clip(soft * 1.6 - 0.2, 0, 1), 0)

    out = np.zeros((h + PAD_TOP, w, 4), np.uint8)
    out[PAD_TOP:, :, :3] = rgb.astype(np.uint8)
    out[PAD_TOP:, :, 3] = (alpha * 255).astype(np.uint8)
    out[out[..., 3] == 0, :3] = 0
    Image.fromarray(out, "RGBA").save(DST, optimize=True)
    print(f"{DST} {w}x{h + PAD_TOP}, kept {keep.sum()} px")

if __name__ == "__main__":
    sys.exit(main())
