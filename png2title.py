#!/usr/bin/env python3
"""
png2title.py - Pack a 4-color PNG (up to 256x256) into a ZX0-compressed
               digger_title.c for the BK-0010 title screen.

Pipeline:
  PNG (<=256x256, BK palette) -> centered on 256x256 black canvas
                              -> raw 16384-byte BK-0010 screen
                              -> zx0 compress
                              -> C array in digger_title.c

Usage:
  ./png2title.py [-i input.png] [-o digger_title.c] [-z /path/to/zx0]

Defaults:
  input  = Digger_cover_05.png
  output = digger_title.c
  zx0    = /home/prcoder/BK0010/lz0/ZX0/src/zx0   (override with $ZX0 or -z)

BK-0010 default palette (mode 0):
  00 = black (0,   0,   0)
  01 = blue  (42,  0,   247)
  10 = green (11,  255, 1)
  11 = red   (255, 0,   0)

Image rules:
  - Width and height must each be <= 256. Smaller images are centered on
    a 256x256 black canvas. Larger ones are rejected.
  - Any RGB triple is mapped to the nearest BK colour above (Euclidean
    distance). For sharp results pre-quantise the picture to exactly
    these four colours in GIMP/etc. - the script handles small export
    artefacts but won't dither a true-colour photograph.
"""

import argparse
import os
import subprocess
import sys
import tempfile

try:
    from PIL import Image
except ImportError:
    sys.exit('Error: Pillow (PIL) is required. Install: pip install Pillow')


# RGB -> BK-0010 2-bit color index
BK_PALETTE = {
    (  0,   0,   0): 0b00,  # black
    ( 42,   0, 247): 0b01,  # blue
    ( 11, 255,   1): 0b10,  # green
    (255,   0,   0): 0b11,  # red
}

DEFAULT_ZX0 = '/home/prcoder/БК0010/lz0/ZX0/src/zx0'  # БК0010


def _nearest_bk(rgb):
    """Map an arbitrary RGB triple to a BK 2-bit index by nearest BK colour
    (Euclidean distance squared). The caller is responsible for feeding an
    image that's already been quantised to the four BK colours - anything
    else gets force-snapped here."""
    best_d = None
    best_v = None
    for bk_rgb, bk_v in BK_PALETTE.items():
        dr = rgb[0] - bk_rgb[0]
        dg = rgb[1] - bk_rgb[1]
        db = rgb[2] - bk_rgb[2]
        d = dr * dr + dg * dg + db * db
        if best_d is None or d < best_d:
            best_d = d
            best_v = bk_v
    return best_v


def png_to_raw(png_path):
    """Read PNG, pad to 256x256 with black, pack to 16384-byte BK raw screen."""
    src = Image.open(png_path)
    w, h = src.size
    if w > 256 or h > 256:
        sys.exit(f'Error: image {w}x{h} exceeds 256x256')

    # Build a 256x256 canvas with black background and paste the source
    # centered. Work in 'P' mode if possible to preserve indexed palette.
    if src.mode == 'P':
        canvas = Image.new('P', (256, 256), 0)
        # Use the source palette so paste keeps indices (black at index 0).
        # The actual mapping to BK happens below, by RGB identity.
        canvas.putpalette(src.getpalette())
    else:
        src = src.convert('RGB')
        canvas = Image.new('RGB', (256, 256), (0, 0, 0))
    pad_x = (256 - w) // 2
    pad_y = (256 - h) // 2
    canvas.paste(src, (pad_x, pad_y))
    if (pad_x, pad_y) != (0, 0):
        print(f'  pad         : {w}x{h} -> 256x256 (offset {pad_x},{pad_y})')

    if canvas.mode == 'P':
        palette = canvas.getpalette()
        idx_to_bk = {i: _nearest_bk(tuple(palette[i * 3:i * 3 + 3]))
                     for i in set(canvas.getdata())}
        pixels = list(canvas.getdata())
        get_bk = idx_to_bk.__getitem__
    else:
        # In RGB mode the image may have many slightly-off shades from PNG
        # export noise (RGB(1,0,0) instead of pure black, near-blue instead
        # of (42,0,247), etc). Cache the snap result per unique RGB.
        cache = {}
        def get_bk(p):
            v = cache.get(p)
            if v is None:
                v = _nearest_bk(p)
                cache[p] = v
            return v
        pixels = list(canvas.getdata())

    out = bytearray()
    for y in range(256):
        row = y * 256
        for x_byte in range(64):
            base = row + x_byte * 4
            b = (get_bk(pixels[base]) |
                 (get_bk(pixels[base + 1]) << 2) |
                 (get_bk(pixels[base + 2]) << 4) |
                 (get_bk(pixels[base + 3]) << 6))
            out.append(b)
    assert len(out) == 16384
    return bytes(out)


def compress_zx0(zx0_bin, raw):
    """Run external zx0 compressor on the raw screen and return compressed bytes."""
    if not os.access(zx0_bin, os.X_OK):
        sys.exit(f'Error: zx0 tool not executable: {zx0_bin}\n'
                 f'  Set $ZX0 or use -z to point at a built zx0 binary.')
    fin_name = fout_name = None
    try:
        with tempfile.NamedTemporaryFile(suffix='.bin', delete=False) as fin:
            fin.write(raw)
            fin_name = fin.name
        fout_name = fin_name + '.zx0'
        result = subprocess.run([zx0_bin, '-f', fin_name, fout_name],
                                capture_output=True, text=True)
        if result.returncode != 0:
            sys.exit(f'Error: zx0 failed:\n{result.stderr or result.stdout}')
        return open(fout_name, 'rb').read()
    finally:
        for f in (fin_name, fout_name):
            if f:
                try: os.unlink(f)
                except FileNotFoundError: pass


def write_c(out_path, data):
    """Emit digger_title.c with cover_zx0_size and cover_zx0[] in the existing format."""
    n = len(data)
    with open(out_path, 'w') as f:
        f.write('#include <stdint.h>\n')
        f.write('#include "digger_title.h"\n\n')
        f.write(f'const uint16_t cover_zx0_size = {n};\n')
        f.write('const uint8_t cover_zx0[] = {\n')
        for i in range(0, n, 16):
            chunk = data[i:i + 16]
            line = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f'    {line}' + (',\n' if i + 16 < n else '\n'))
        f.write('};\n')


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('-i', '--input', default='Digger_cover_05.png',
                    help='input PNG (default: Digger_cover_05.png)')
    ap.add_argument('-o', '--output', default='digger_title.c',
                    help='output C file (default: digger_title.c)')
    ap.add_argument('-z', '--zx0', default=os.environ.get('ZX0', DEFAULT_ZX0),
                    help='path to zx0 binary (or $ZX0)')
    args = ap.parse_args()

    if not os.path.isfile(args.input):
        sys.exit(f'Error: input PNG not found: {args.input}')

    raw = png_to_raw(args.input)
    print(f'  PNG  -> raw : {len(raw):>6} bytes')
    zx0 = compress_zx0(args.zx0, raw)
    ratio = 100.0 * len(zx0) / len(raw)
    print(f'  raw  -> zx0 : {len(zx0):>6} bytes  ({ratio:.1f}% of raw)')
    write_c(args.output, zx0)
    print(f'  zx0  -> C   : {args.output}  (cover_zx0_size = {len(zx0)})')


if __name__ == '__main__':
    main()
