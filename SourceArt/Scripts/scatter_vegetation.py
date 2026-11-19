"""
Project Eldoria - vegetation / rock scattering for Tharum's jungle.
Reads Generated/Terrain/heightmap.npy, writes Generated/Terrain/scatter.json.
Coordinates in meters (UE: X north, Y east). Gameplay zones are kept clear.
"""
import json
import math
import os
import sys

import numpy as np

here = os.path.dirname(os.path.abspath(__file__))
terrain_dir = os.path.join(here, "..", "Generated", "Terrain")
X, Y, H = np.load(os.path.join(terrain_dir, "heightmap.npy"))
X_MIN, Y_MIN, RES = float(X[0, 0]), float(Y[0, 0]), float(X[1, 0] - X[0, 0])
rng = np.random.default_rng(42)

# --- same playable floor as the terrain generator
ellipse = (X / 50.0) ** 2 + (Y / 45.0) ** 2
clearing = ellipse <= 1.0
plateau = (X >= -100.0) & (X <= -42.0) & (np.abs(Y) <= 7.0)
ramp = (X > -42.0) & (X < -25.0) & (Y >= 1.0) & (Y <= 7.0)
tunnel = (X >= 44.0) & (X <= 62.0) & (np.abs(Y) <= 3.0)
nook = (X >= 62.0) & (X <= 82.0) & (np.abs(Y) <= 12.0)
floor = clearing | plateau | ramp | tunnel | nook


def grid_distance(mask, max_steps=60):
    """Approximate distance (m) from every cell to the nearest cell where mask is True."""
    dist = np.where(mask, 0.0, np.inf)
    for _ in range(max_steps):
        best = dist.copy()
        for di, dj, w in [(1, 0, 1), (-1, 0, 1), (0, 1, 1), (0, -1, 1), (1, 1, 1.414), (1, -1, 1.414), (-1, 1, 1.414), (-1, -1, 1.414)]:
            shifted = np.roll(np.roll(dist, di, 0), dj, 1) + w * RES
            best = np.minimum(best, shifted)
        if np.array_equal(best, dist):
            break
        dist = best
    return dist


dist_out = grid_distance(floor)      # outside the floor: distance to it
dist_in = grid_distance(~floor)      # inside the floor: distance to its edge

gx, gy = np.gradient(H, RES, RES)
slope = np.degrees(np.arctan(np.hypot(gx, gy)))


def sample(field, x, y):
    i = (x - X_MIN) / RES
    j = (y - Y_MIN) / RES
    i0, j0 = int(np.clip(math.floor(i), 0, field.shape[0] - 2)), int(np.clip(math.floor(j), 0, field.shape[1] - 2))
    fi, fj = i - i0, j - j0
    return float(field[i0, j0] * (1 - fi) * (1 - fj) + field[i0 + 1, j0] * fi * (1 - fj)
                 + field[i0, j0 + 1] * (1 - fi) * fj + field[i0 + 1, j0 + 1] * fi * fj)


# --- gameplay zones that must stay clear of trees, bushes and rocks (x, y, radius)
CIRCLES = [
    (2, -30, 11), (12, -30, 9), (22, -30, 6),     # tower + its fall lane to the north
    (-5, -30, 7),                                  # scaffold
    (33, 2, 9),                                    # pool (Tharum drinks here)
    (2, -18, 9), (2, -10, 8),                      # where the tower falls onto Tharum
    (22, -18, 11),                                 # colonnade
    (-31.5, -1.5, 6),                              # hut
    (49, 0, 8),                                    # tunnel mouth + boulder
    (-33, 4, 6), (-42, 0, 7),                      # arrival ramp and overlook
]
RECTS = [
    (8, 36, 3, 46),       # stairs + aqueduct + block (xmin, xmax, ymin, ymax)
    (27, 41, -6, 30),     # Tharum's drinking spot beside the aqueduct
    (42, 66, -8, 8),      # north tunnel: only the FORCE boulder may block it
]
CENTER_LANE = 26.0        # radius around the clearing centre kept open for charges (no trees/bushes)


def blocked(x, y, extra=0.0):
    for cx, cy, r in CIRCLES:
        if (x - cx) ** 2 + (y - cy) ** 2 < (r + extra) ** 2:
            return True
    for x0, x1, y0, y1 in RECTS:
        if x0 - extra < x < x1 + extra and y0 - extra < y < y1 + extra:
            return True
    return False


def poisson(count, spacing, accept, tries=60000, existing=None):
    pts = list(existing or [])
    out = []
    for _ in range(tries):
        if len(out) >= count:
            break
        x = rng.uniform(X_MIN + 2, float(X[-1, 0]) - 2)
        y = rng.uniform(Y_MIN + 2, float(Y[0, -1]) - 2)
        if not accept(x, y):
            continue
        if any((x - px) ** 2 + (y - py) ** 2 < spacing ** 2 for px, py in pts):
            continue
        pts.append((x, y))
        out.append((x, y))
    return out


def inst(mesh, x, y, scale, sink=0.0, yaw=None, collider=None):
    return {"mesh": mesh, "x": round(x, 2), "y": round(y, 2), "z": round(sample(H, x, y) - sink, 2),
            "yaw": round(float(rng.uniform(0, 360)) if yaw is None else yaw, 1),
            "scale": round(scale, 3), **({"collider": collider} if collider else {})}


result = {"trees_hero": [], "foliage": {}, "rocks": []}
TREES = ["CommonTree_1", "CommonTree_2", "CommonTree_3", "CommonTree_4", "CommonTree_5"]


def add_foliage(name, items):
    result["foliage"].setdefault(name, []).extend(items)


# 1. Hero trees inside the clearing, near its edge (big, with trunk colliders)
hero = poisson(16, 13.0, lambda x, y: clearing[int((x - X_MIN) / RES), int((y - Y_MIN) / RES)]
               and 2.0 < sample(dist_in, x, y) < 9.0 and not blocked(x, y, 4.0) and math.hypot(x, y) > CENTER_LANE)
for x, y in hero:
    s = float(rng.uniform(3.2, 4.4))
    result["trees_hero"].append(inst(rng.choice(TREES), x, y, s, sink=0.3, collider={"radius": round(0.07 * 7.3 * s / 2 + 0.3, 2), "height": round(7.3 * s * 0.45, 2)}))

# 2. Canopy trees on the cliff tops all around (no collision, unreachable)
canopy = poisson(420, 6.5, lambda x, y: 3.0 < sample(dist_out, x, y) < 40.0 and sample(slope, x, y) < 30.0)
for x, y in canopy:
    add_foliage(str(rng.choice(TREES)), [inst("", x, y, float(rng.uniform(2.6, 4.2)), sink=0.5)])

# 3. Big plants and ferns along the clearing edge and around hero trees
def edge_plant(x, y):
    i, j = int((x - X_MIN) / RES), int((y - Y_MIN) / RES)
    if not floor[i, j] or blocked(x, y, 1.0):
        return False
    d = sample(dist_in, x, y)
    near_hero = any((x - hx) ** 2 + (y - hy) ** 2 < 49 for hx, hy in hero)
    return d < 10.0 or near_hero or (math.hypot(x, y) > CENTER_LANE and rng.random() < 0.08)

for name, count, smin, smax in [("Plant_1_Big", 150, 0.4, 0.65), ("Plant_7_Big", 160, 1.2, 2.0), ("Fern_1", 280, 0.28, 0.42), ("Bush_Common_Flowers", 70, 0.65, 0.9)]:
    pts = poisson(count, 1.8, edge_plant)
    add_foliage(name, [inst("", x, y, float(rng.uniform(smin, smax)), sink=0.05) for x, y in pts])

# 4. Grass everywhere on the floor except dirt and structure footprints (dense, small)
def grassy(x, y):
    i, j = int((x - X_MIN) / RES), int((y - Y_MIN) / RES)
    if not floor[i, j] or tunnel[i, j]:
        return False
    if (x - 33) ** 2 + (y - 2) ** 2 < 7.5 ** 2:          # pool + banks
        return False
    path = abs(y - 4.0 * math.sin(x / 9.0)) < 1.8 and x < 5.0
    if path or plateau[i, j] and abs(y) < 3.0:
        return False
    if any(x0 < x < x1 and y0 < y < y1 for x0, x1, y0, y1 in [(19, 25, 12, 40), (24, 28, 5, 44), (28, 32, 20, 24), (-2, 6, -34, -26)]):
        return False
    return True

for name, count, smin, smax in [("Grass_Common_Short", 6000, 0.35, 0.5), ("Grass_Common_Tall", 1500, 0.42, 0.52), ("Grass_Wispy_Short", 2500, 0.38, 0.55), ("Grass_Wispy_Tall", 900, 0.42, 0.54)]:
    pts = poisson(count, 0.6, grassy, tries=300000)
    add_foliage(name, [inst("", x, y, float(rng.uniform(smin, smax)), sink=0.02) for x, y in pts])

# 5. Flower and clover patches
centers = poisson(40, 8.0, grassy)
patch = []
for cx, cy in centers:
    for _ in range(rng.integers(6, 14)):
        x, y = cx + rng.normal(0, 1.6), cy + rng.normal(0, 1.6)
        if grassy(x, y):
            patch.append((x, y))
names = ["Flower_3_Group", "Flower_4_Group", "Clover_1", "Clover_2"]
for k, (x, y) in enumerate(patch):
    add_foliage(names[k % 4], [inst("", x, y, float(rng.uniform(0.25, 0.38) if k % 4 < 2 else rng.uniform(0.22, 0.35)))])

# 6. Mushrooms at the foot of hero trees
for hx, hy in hero:
    for _ in range(rng.integers(2, 5)):
        a, r = rng.uniform(0, 2 * math.pi), rng.uniform(2.0, 4.0)
        add_foliage("Mushroom_Common", [inst("", hx + r * math.cos(a), hy + r * math.sin(a), float(rng.uniform(0.8, 1.4)))])

# 7. Rocks: at the foot of the cliffs (collision) and boulders on top
ROCKS = ["Rock_Medium_1", "Rock_Medium_2", "Rock_Medium_3"]
base_rocks = poisson(55, 6.0, lambda x, y: floor[int((x - X_MIN) / RES), int((y - Y_MIN) / RES)]
                     and sample(dist_in, x, y) < 2.5 and not blocked(x, y, 2.0))
for x, y in base_rocks:
    result["rocks"].append(inst(str(rng.choice(ROCKS)), x, y, float(rng.uniform(1.5, 3.5)), sink=0.6))
top_rocks = poisson(45, 9.0, lambda x, y: 4.0 < sample(dist_out, x, y) < 25.0 and sample(slope, x, y) < 26.0)
for x, y in top_rocks:
    result["rocks"].append(inst(str(rng.choice(ROCKS)), x, y, float(rng.uniform(3.0, 6.5)), sink=1.2))

# Pebbles along the dirt path (no collision)
path_pts = poisson(90, 1.5, lambda x, y: abs(y - 4.0 * math.sin(x / 9.0)) < 2.6 and -45 < x < 5 and floor[int((x - X_MIN) / RES), int((y - Y_MIN) / RES)])
for k, (x, y) in enumerate(path_pts):
    add_foliage("Pebble_Round_%d" % (k % 5 + 1), [inst("", x, y, float(rng.uniform(1.0, 2.2)))])

with open(os.path.join(terrain_dir, "scatter.json"), "w") as f:
    json.dump(result, f)
print("hero trees", len(result["trees_hero"]), "rocks", len(result["rocks"]))
print({k: len(v) for k, v in result["foliage"].items()})
