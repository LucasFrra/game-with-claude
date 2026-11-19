# Project Eldoria - BOTW-like vegetation and backdrop, generated headless:
#   blender -b --factory-startup --python blender_botw_assets.py
# Outputs glTF files to SourceArt/Generated/Foliage and SourceArt/Generated/Backdrop.
#
# Key tricks of the style:
#  - grass: thin curved blades whose normals all point UP, so the tuft is lit exactly like the ground (dense carpet)
#  - trees: rounded canopies (solid core + leaf cards) whose normals are "spherized" from the canopy centre,
#    giving the soft, painterly light/shadow split of BOTW trees instead of faceted polygons
#  - vertex colour R = gradient (root->tip for grass, inside->outside for leaves): darkening + wind weight
import math
import os
import random

import bmesh
import bpy
from mathutils import Vector, Matrix, noise

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
OUT_FOLIAGE = os.path.join(ROOT, "Generated", "Foliage")
OUT_BACKDROP = os.path.join(ROOT, "Generated", "Backdrop")
os.makedirs(OUT_FOLIAGE, exist_ok=True)
os.makedirs(OUT_BACKDROP, exist_ok=True)


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def material(name):
    return bpy.data.materials.get(name) or bpy.data.materials.new(name)


class MeshBuilder:
    """Collects verts/faces with per-loop colour, UV and custom normal, then builds one object."""

    def __init__(self):
        self.verts, self.faces = [], []      # faces: (indices, material_index)
        self.vcol, self.vnorm, self.vuv = [], [], []

    def v(self, co, col=1.0, nrm=(0, 0, 1), uv=(0, 0)):
        self.verts.append(tuple(co))
        self.vcol.append(col)
        self.vnorm.append(tuple(Vector(nrm).normalized()))
        self.vuv.append(uv)
        return len(self.verts) - 1

    def f(self, idx, mat=0):
        self.faces.append((tuple(idx), mat))

    def build(self, name, materials):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(self.verts, [], [f for f, _ in self.faces])
        for poly, (_, m) in zip(mesh.polygons, self.faces):
            poly.material_index = m
            poly.use_smooth = True
        col = mesh.color_attributes.new("Col", "FLOAT_COLOR", "CORNER")
        uv = mesh.uv_layers.new(name="UVMap")
        normals = []
        for loop in mesh.loops:
            vi = loop.vertex_index
            c = self.vcol[vi]
            col.data[loop.index].color = (c, c, c, 1.0) if isinstance(c, float) else c
            uv.data[loop.index].uv = self.vuv[vi]
            normals.append(self.vnorm[vi])
        mesh.normals_split_custom_set(normals)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        for m in materials:
            mesh.materials.append(material(m))
        return obj


def export(obj, folder):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(folder, obj.name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_yup=True,
                              export_apply=True, export_normals=True, export_vertex_color="ACTIVE",
                              export_all_vertex_colors=False)
    print("exported", path, len(obj.data.vertices), "verts")


# ------------------------------------------------------------------ grass
def grass_clump(name, blades, h_range, radius, seed):
    random.seed(seed)
    b = MeshBuilder()
    for _ in range(blades):
        a = random.uniform(0, math.tau)
        r = radius * math.sqrt(random.random())
        base = Vector((math.cos(a) * r, math.sin(a) * r, 0))
        h = random.uniform(*h_range)
        w = random.uniform(0.022, 0.034)
        yaw = random.uniform(0, math.tau)
        side = Vector((math.cos(yaw), math.sin(yaw), 0))
        lean_dir = Vector((-side.y, side.x, 0)) * random.choice((-1, 1))
        lean = random.uniform(0.15, 0.45)
        tone = random.uniform(0.0, 1.0)          # G: per-blade variation
        segs = 4
        rows = []
        for s in range(segs + 1):
            t = s / segs
            center = base + Vector((0, 0, h * t)) + lean_dir * (lean * h * t * t)
            half = w * (1 - t) ** 0.85
            col = (t, tone, 0.0, 1.0)
            if s == segs:
                rows.append([b.v(center, col, (0, 0, 1), (0.5, 1))])
            else:
                rows.append([b.v(center - side * half, col, (0, 0, 1), (0, t)), b.v(center + side * half, col, (0, 0, 1), (1, t))])
        for s in range(segs):
            lo, hi = rows[s], rows[s + 1]
            if len(hi) == 1:
                b.f((lo[0], lo[1], hi[0]))
            else:
                b.f((lo[0], lo[1], hi[1], hi[0]))
    return b.build(name, ["Grass"])


# ------------------------------------------------------------------ trees
def tube(b, spine, radii, mat, ring=10, col=0.0):
    """Tapered tube along a list of points; returns nothing."""
    rings = []
    for i, p in enumerate(spine):
        d = (spine[min(i + 1, len(spine) - 1)] - spine[max(i - 1, 0)]).normalized()
        ref = Vector((0, 0, 1)) if abs(d.z) < 0.9 else Vector((1, 0, 0))
        u = d.cross(ref).normalized()
        v = d.cross(u).normalized()
        row = []
        for k in range(ring):
            a = k / ring * math.tau
            n = u * math.cos(a) + v * math.sin(a)
            row.append(b.v(p + n * radii[i], (col, 0.5, 0, 1), n, (k / ring, i / len(spine))))
        rings.append(row)
    for i in range(len(rings) - 1):
        for k in range(ring):
            a0, a1 = rings[i][k], rings[i][(k + 1) % ring]
            b0, b1 = rings[i + 1][k], rings[i + 1][(k + 1) % ring]
            b.f((a0, a1, b1, b0), mat)


def curve(p0, p1, bend, steps):
    pts = []
    for i in range(steps + 1):
        t = i / steps
        p = p0.lerp(p1, t)
        p += bend * math.sin(math.pi * t)
        pts.append(p)
    return pts


def canopy_blob(b, center, radius, canopy_center, canopy_radius, cards, seed):
    """Solid displaced core + leaf cards; normals spherized from the whole canopy centre."""
    random.seed(seed)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=radius * 0.82)
    for vert in bm.verts:
        n = vert.co.normalized()
        vert.co += n * noise.noise(vert.co * 0.45 + Vector((seed, 0, 0))) * radius * 0.18
    bm.verts.ensure_lookup_table()
    base = len(b.verts)
    for vert in bm.verts:
        p = center + vert.co
        sph = (p - canopy_center)
        ao = min(1.0, 0.35 + 0.65 * sph.length / canopy_radius)
        b.v(p, (ao, 0.5, 0, 1), sph, (0.5, 0.5))
    for face in bm.faces:
        b.f(tuple(base + v.index for v in face.verts), 2)       # slot 2: solid core
    bm.free()
    # leaf cards scattered on the blob surface, facing outwards-ish
    for _ in range(cards):
        dirv = Vector((random.gauss(0, 1), random.gauss(0, 1), random.gauss(0, 1) + 0.35)).normalized()
        p = center + dirv * radius * random.uniform(0.82, 1.05)
        size = radius * random.uniform(0.55, 0.8)
        up = Vector((0, 0, 1))
        t1 = dirv.cross(up if abs(dirv.z) < 0.95 else Vector((1, 0, 0))).normalized()
        t2 = dirv.cross(t1).normalized()
        rot = random.uniform(0, math.tau)
        t1, t2 = t1 * math.cos(rot) + t2 * math.sin(rot), -t1 * math.sin(rot) + t2 * math.cos(rot)
        corners = [p + (t1 * sx + t2 * sy) * size * 0.5 for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        uvs = [(0, 1), (1, 1), (1, 0), (0, 0)]
        idx = []
        for c, uv in zip(corners, uvs):
            sph = c - canopy_center
            ao = min(1.0, 0.35 + 0.65 * sph.length / canopy_radius)
            idx.append(b.v(c, (ao, 0.5, 0, 1), sph, uv))
        b.f(idx, 1)                                              # slot 1: leaf cards


def tree(name, height, trunk_r, blobs, seed, lean=0.0):
    random.seed(seed)
    b = MeshBuilder()
    top = Vector((lean * height * 0.12, 0, height * 0.48))
    trunk = curve(Vector((0, 0, -0.4)), top, Vector((lean * height * 0.06, height * 0.025, 0)), 8)
    # flared root, then a slow taper
    radii = [trunk_r * (1.6 if i == 0 else 1.15 if i == 1 else 1.0) * (1 - 0.35 * i / 8) for i in range(9)]
    tube(b, trunk, radii, 0, ring=14)
    canopy_center = top + Vector((0, 0, height * 0.2))
    canopy_radius = height * 0.38
    centers = [(canopy_center + Vector((0, 0, height * 0.12)), height * 0.22)]
    # lower ring of big clumps, on thick branches
    for k in range(blobs):
        a = k / blobs * math.tau + random.uniform(-0.25, 0.25)
        off = Vector((math.cos(a), math.sin(a), 0)) * height * random.uniform(0.2, 0.27)
        off.z = random.uniform(-0.06, 0.02) * height
        centers.append((canopy_center + off, height * random.uniform(0.17, 0.21)))
        tube(b, curve(top, canopy_center + off * 0.8, Vector((0, 0, height * 0.05)), 4),
             [trunk_r * 0.62 * (1 - 0.55 * i / 4) for i in range(5)], 0, ring=8)
    # upper ring: smaller clumps that round the crown off
    for k in range(3):
        a = k / 3 * math.tau + 0.5 + random.uniform(-0.2, 0.2)
        off = Vector((math.cos(a), math.sin(a), 0)) * height * 0.12
        off.z = height * random.uniform(0.1, 0.16)
        centers.append((canopy_center + off, height * random.uniform(0.15, 0.18)))
    for i, (c, r) in enumerate(centers):
        canopy_blob(b, c, r, canopy_center, canopy_radius, cards=40, seed=seed * 10 + i)
    return b.build(name, ["Bark", "Leaves", "LeavesCore"])


def bush(name, radius, seed):
    random.seed(seed)
    b = MeshBuilder()
    center = Vector((0, 0, radius * 0.55))
    for i in range(3):
        a = i / 3 * math.tau
        off = Vector((math.cos(a), math.sin(a), 0)) * radius * 0.45
        canopy_blob(b, center + off, radius * 0.62, center, radius * 1.2, cards=18, seed=seed * 7 + i)
    canopy_blob(b, center + Vector((0, 0, radius * 0.3)), radius * 0.6, center, radius * 1.2, cards=18, seed=seed * 7 + 9)
    # dummy bark triangle keeps the 3-slot layout identical to the trees (hidden under the bush)
    v0 = b.v((0, 0, 0)); v1 = b.v((0.01, 0, 0)); v2 = b.v((0, 0.01, 0))
    b.f((v0, v1, v2), 0)
    return b.build(name, ["Bark", "Leaves", "LeavesCore"])


# ------------------------------------------------------------------ distant peaks
def peak(name, height, base_radius, seed):
    random.seed(seed)
    b = MeshBuilder()
    rings, seg = 18, 48
    grid = []
    for i in range(rings + 1):
        t = i / rings                         # 0 summit .. 1 foot
        row = []
        for k in range(seg):
            a = k / seg * math.tau
            ridge = 1 + 0.35 * noise.noise(Vector((math.cos(a) * 2.2, math.sin(a) * 2.2, seed))) + 0.15 * math.sin(a * 3 + seed)
            r = base_radius * (t ** 0.85) * ridge
            z = height * (1 - t) ** 1.6 * (1 + 0.06 * noise.noise(Vector((math.cos(a) * 6, math.sin(a) * 6, t * 4 + seed))))
            p = Vector((math.cos(a) * r, math.sin(a) * r, z))
            snow = 1.0 if z > height * 0.72 else 0.0
            row.append(b.v(p, (snow, 0.5, 0, 1), (math.cos(a) * 0.6, math.sin(a) * 0.6, 0.8), (k / seg, t)))
        grid.append(row)
    for i in range(rings):
        for k in range(seg):
            a0, a1 = grid[i][k], grid[i][(k + 1) % seg]
            b0, b1 = grid[i + 1][k], grid[i + 1][(k + 1) % seg]
            b.f((a0, b0, b1, a1))
    obj = b.build(name, ["Mountain"])
    # real normals for distant shapes (soft, flat-ish)
    obj.data.free_normals_split() if hasattr(obj.data, "free_normals_split") else None
    return obj


reset()
export(grass_clump("SM_Grass_Clump_A", 34, (0.22, 0.42), 0.28, 1), OUT_FOLIAGE)
reset()
export(grass_clump("SM_Grass_Clump_B", 26, (0.40, 0.75), 0.30, 2), OUT_FOLIAGE)
reset()
export(tree("SM_Tree_Broadleaf_A", 15.0, 0.95, 6, seed=3), OUT_FOLIAGE)
reset()
export(tree("SM_Tree_Broadleaf_B", 9.0, 0.55, 5, seed=5, lean=0.6), OUT_FOLIAGE)
reset()
export(bush("SM_Bush_Round_A", 1.4, seed=11), OUT_FOLIAGE)
for i, (h, r) in enumerate([(900, 1500), (1300, 2100), (700, 1300), (1100, 1800)]):
    reset()
    export(peak("SM_Peak_%d" % i, h, r, seed=20 + i), OUT_BACKDROP)
