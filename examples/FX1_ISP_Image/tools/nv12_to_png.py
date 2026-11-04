"""Decode this example's packed 2688x1520 full-range BT.601 NV12 dump."""
import argparse
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('input', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
w, h = 2688, 1520
data = args.input.read_bytes()
if len(data) != w * h * 3 // 2:
    parser.error('Expected exactly 6128640 bytes of packed NV12')
y = Image.frombytes('L', (w, h), data[:w*h])
uv = data[w*h:]
u = Image.frombytes('L', (w//2, h//2), uv[0::2]).resize((w, h), Image.Resampling.NEAREST)
v = Image.frombytes('L', (w//2, h//2), uv[1::2]).resize((w, h), Image.Resampling.NEAREST)
Image.merge('YCbCr', (y, u, v)).convert('RGB').save(args.output)
