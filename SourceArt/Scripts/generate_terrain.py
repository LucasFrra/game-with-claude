"""
Project Eldoria - Tharum's jungle terrain generator.

Builds a heightfield mesh that keeps every gameplay height of the greybox (arrival plateau 6 m,
clearing at 0, ramp, mounds, pool, north tunnel and shrine nook) and replaces the box cliffs with
sculpted natural cliffs. Exports a glTF (meters, Y-up) with vertex colors:
    R = rock (steep slopes), G = dirt (paths, pool banks), B = moss / grass variation.

Usage: python generate_terrain.py <out_dir>
World convention (UE): X = north, Y = east, Z = up, meters here.
"""
import json
import math
import os
import struct
import sys

import numpy as np

RES = 1.0                       # grid spacing (m)
X_MIN, X_MAX = -125.0, 112.0
Y_MIN, Y_MAX = -78.0, 78.0
SEED = 7

xs = np.arange(X_MIN, X_MAX + RES * 0.5, RES)
ys = np.arange(Y_MIN, Y_MAX + RES * 0.5, RES)
X, Y = np.meshgrid(xs, ys, indexing="ij")   # X[i, j], Y[i, j]


# ---------------------------------------------------------------- noise
def value_noise(x, y, scale, seed):
    rng = np.random.default_rng(seed)
    gx, gy = x / scale, y / scale
    x0, y0 = np.floor(gx).astype(int), np.floor(gy).astype(int)
    fx, fy = gx - x0, gy - y0
    size = 512
    table = rng.random((size, size))
    def at(ix, iy):
        return table[ix % size, iy % size]
    sx = fx * fx * (3 - 2 * fx)
    sy = fy * fy * (3 - 2 * fy)
    a = at(x0, y0) * (1 - sx) + at(x0 + 1, y0) * sx
    b = at(x0, y0 + 1) * (1 - sx) + at(x0 + 1, y0 + 1) * sx
    return a * (1 - sy) + b * sy


def fbm(x, y, scale, octaves, seed):
    total, amp, norm = 0.0, 1.0, 0.0
    for o in range(octaves):
        total = total + value_noise(x, y, scale / (2 ** o), seed + o * 31) * amp
        norm += amp
        amp *= 0.5
    return total / norm


def smoothstep(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3 - 2 * t)


# ---------------------------------------------------------------- playable floor
ellipse = (X / 50.0) ** 2 + (Y / 45.0) ** 2
clearing = ellipse <= 1.0
plateau = (X >= -100.0) & (X <= -42.0) & (np.abs(Y) <= 7.0)
ramp = (X > -42.0) & (X < -25.0) & (Y >= 1.0) & (Y <= 7.0)
tunnel = ((X >= 44.0) & (X <= 64.0) & (np.abs(Y) <= 3.0)) | ((X >= 58.0) & (X <= 64.0) & (np.abs(Y) <= 4.5))  # flared exit: the FORCE boulder slides out
nook = (X >= 62.0) & (X <= 82.0) & (np.abs(Y) <= 12.0)

floor_mask = clearing | plateau | ramp | tunnel | nook

floor_h = np.zeros_like(X)
floor_h[plateau] = 6.0
ramp_t = np.clip((X - (-42.0)) / 17.0, 0.0, 1.0)
floor_h[ramp] = (6.0 * (1.0 - ramp_t))[ramp]

# Gentle mounds (match the greybox cones), smooth bumps instead of facets
for cx, cy, r, h in [(-25.0, -18.0, 13.0, 3.0), (-12.0, 26.0, 10.0, 2.5)]:
    d = np.hypot(X - cx, Y - cy)
    bump = h * np.clip(1.0 - (d / r) ** 2, 0.0, 1.0) ** 1.5
    floor_h = np.where(clearing, floor_h + bump, floor_h)

# Pool: a real shallow basin under the aqueduct's broken end
pd = np.hypot(X - 33.0, Y - 2.0)
basin = -0.9 * np.clip(1.0 - (pd / 6.5) ** 2, 0.0, 1.0)
floor_h = np.where(clearing, floor_h + basin, floor_h)

# Subtle ground variation, kept flat around gameplay structures so they sit level
flat_zones = [
    (2.0, -30.0, 10.0),    # tower + scaffold
    (22.0, 26.0, 16.0),    # stairs + aqueduct + lift
    (33.0, 2.0, 8.5),      # pool (Tharum drinks here)
    (33.5, 17.0, 11.0),    # Tharum's drinking spot beside the aqueduct
    (22.0, -18.0, 11.0),   # colonnade
    (-31.5, -1.5, 5.0),    # hut
]
flat = np.zeros_like(X)
for cx, cy, r in flat_zones:
    flat = np.maximum(flat, 1.0 - smoothstep(r * 0.7, r, np.hypot(X - cx, Y - cy)))
ground_noise = (fbm(X, Y, 18.0, 3, SEED) - 0.5) * 0.5
floor_h = np.where(clearing & ~plateau & ~ramp, floor_h + ground_noise * (1.0 - flat), floor_h)


# ---------------------------------------------------------------- distance to floor (chamfer)
def chamfer(mask, values):
    """Distance (m) to the nearest floor cell, and that cell's height."""
    big = 1e9
    dist = np.where(mask, 0.0, big)
    near = np.where(mask, values, 0.0)
    nx, ny = dist.shape
    d1, d2 = RES, RES * math.sqrt(2.0)
    passes = [
        (range(nx), range(ny), [(-1, -1, d2), (-1, 0, d1), (-1, 1, d2), (0, -1, d1)]),
        (range(nx - 1, -1, -1), range(ny - 1, -1, -1), [(1, 1, d2), (1, 0, d1), (1, -1, d2), (0, 1, d1)]),
    ]
    for _ in range(2):
        for irange, jrange, kernel in passes:
            for i in irange:
                row = dist[i]
                for j in jrange:
                    best = row[j]
                    if best == 0.0:
                        continue
                    for di, dj, w in kernel:
                        ii, jj = i + di, j + dj
                        if 0 <= ii < nx and 0 <= jj < ny:
                            cand = dist[ii, jj] + w
                            if cand < best:
                                best = cand
                                near[i, j] = near[ii, jj]
                    row[j] = best
    return dist, near


print("distance field...")
dist, base = chamfer(floor_mask, floor_h)

# ---------------------------------------------------------------- cliffs and hills
angle = np.arctan2(Y, X)
cliff_top = 24.0 + 5.0 * np.sin(angle * 3.0 + 0.7) + 3.0 * np.sin(angle * 7.0) \
    + (fbm(X, Y, 40.0, 3, SEED + 5) - 0.5) * 10.0
cliff_top = np.maximum(cliff_top, 18.0)

# Steep face over the first meters, then rounded hill tops with ledges
face = smoothstep(0.0, 5.0, dist) ** 0.65
ledges = face
hill = (fbm(X, Y, 25.0, 4, SEED + 9) - 0.5) * 6.0 * smoothstep(6.0, 20.0, dist)
outside = base + (cliff_top - base) * ledges + hill

height = np.where(floor_mask, floor_h, np.maximum(outside, floor_h))


# ---------------------------------------------------------------- normals & vertex colors
gx, gy = np.gradient(height, RES, RES)
nrm = np.stack([-gx, -gy, np.ones_like(height)], axis=-1)
nrm /= np.linalg.norm(nrm, axis=-1, keepdims=True)
slope = np.degrees(np.arccos(np.clip(nrm[..., 2], -1.0, 1.0)))

rock = smoothstep(32.0, 46.0, slope)
dirt = np.zeros_like(height)
dirt = np.maximum(dirt, (plateau | ramp | tunnel).astype(float) * 0.85)
dirt = np.maximum(dirt, smoothstep(8.5, 5.5, pd) * 0.9)                         # pool banks
path = np.abs(Y - 4.0 * np.sin(X / 9.0)) < 1.6                                   # trodden path
dirt = np.maximum(dirt, (path & clearing & (X < 5.0)).astype(float) * 0.6)
dirt *= (1.0 - rock)
moss = fbm(X, Y, 12.0, 3, SEED + 21)

colors = np.stack([rock, dirt, moss, np.ones_like(rock)], axis=-1)
colors = (np.clip(colors, 0.0, 1.0) * 255.0).astype(np.uint8)


# ---------------------------------------------------------------- mesh + glTF (Y-up, meters)
nx, ny = height.shape
# UE (x, y, z) -> glTF (x, z, y). Verified in editor: UE's glTF import maps glTF Z to UE +Y.
pos = np.stack([X, height, Y], axis=-1).reshape(-1, 3).astype(np.float32)
nor = np.stack([nrm[..., 0], nrm[..., 2], nrm[..., 1]], axis=-1).reshape(-1, 3).astype(np.float32)
uv = np.stack([X / 4.0, Y / 4.0], axis=-1).reshape(-1, 2).astype(np.float32)
col = colors.reshape(-1, 4)

idx = []
for i in range(nx - 1):
    a = i * ny + np.arange(ny - 1)
    b = a + 1
    c = a + ny
    d = c + 1
    # Winding verified in editor: front faces point up after import
    idx.append(np.stack([a, b, c, b, d, c], axis=-1))
indices = np.concatenate(idx).reshape(-1).astype(np.uint32)

out_dir = sys.argv[1] if len(sys.argv) > 1 else "."
os.makedirs(out_dir, exist_ok=True)
name = "SM_Terrain_TharumJungle"
blobs = [pos.tobytes(), nor.tobytes(), uv.tobytes(), col.tobytes(), indices.tobytes()]
offsets, cursor = [], 0
for blob in blobs:
    offsets.append(cursor)
    cursor += len(blob)
    cursor += (-cursor) % 4
buffer = bytearray()
for blob in blobs:
    buffer += blob
    buffer += b"\0" * ((-len(buffer)) % 4)
with open(os.path.join(out_dir, name + ".bin"), "wb") as f:
    f.write(buffer)

count = pos.shape[0]
gltf = {
    "asset": {"version": "2.0", "generator": "Eldoria terrain generator"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{"mesh": 0, "name": name}],
    "materials": [{"name": "M_Terrain", "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1], "metallicFactor": 0, "roughnessFactor": 0.9}}],
    "meshes": [{"name": name, "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2, "COLOR_0": 3}, "indices": 4, "material": 0}]}],
    "buffers": [{"uri": name + ".bin", "byteLength": len(buffer)}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": offsets[0], "byteLength": len(blobs[0]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[1], "byteLength": len(blobs[1]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[2], "byteLength": len(blobs[2]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[3], "byteLength": len(blobs[3]), "target": 34962},
        {"buffer": 0, "byteOffset": offsets[4], "byteLength": len(blobs[4]), "target": 34963},
    ],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": count, "type": "VEC3",
         "min": pos.min(axis=0).tolist(), "max": pos.max(axis=0).tolist()},
        {"bufferView": 1, "componentType": 5126, "count": count, "type": "VEC3"},
        {"bufferView": 2, "componentType": 5126, "count": count, "type": "VEC2"},
        {"bufferView": 3, "componentType": 5121, "normalized": True, "count": count, "type": "VEC4"},
        {"bufferView": 4, "componentType": 5125, "count": int(indices.size), "type": "SCALAR"},
    ],
}
with open(os.path.join(out_dir, name + ".gltf"), "w") as f:
    json.dump(gltf, f)

# Quick sanity report
print("grid", height.shape, "verts", count, "tris", indices.size // 3)
for label, (px, py) in {"start": (-92, 0), "clearing": (0, 0), "pool": (26, 1), "tower": (2, -30),
                        "ramp_mid": (-33, 4), "tunnel": (55, 0), "nook": (74, 0), "east_cliff": (0, 52),
                        "west_cliff": (0, -52), "north_cliff": (52, 10)}.items():
    i = int(round((px - X_MIN) / RES)); j = int(round((py - Y_MIN) / RES))
    print("%-11s h=%6.2f slope=%5.1f" % (label, height[i, j], slope[i, j]))
np.save(os.path.join(out_dir, "heightmap.npy"), np.stack([X, Y, height], axis=0))
