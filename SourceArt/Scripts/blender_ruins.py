# Project Eldoria - J3 ruins kit, generated headless from the greybox inventory:
#   blender -b --factory-startup --python blender_ruins.py
# Reads Generated/Ruins/greybox.json (export_greybox.py) and writes one glTF per distinct piece, at the exact
# size of the greybox it replaces (centred pivot like Unreal's basic shapes), so gameplay geometry is unchanged.
#   stone blocks: bevelled, gently warped, chipped corners       wood: planks with soft bevels
#   stone columns: drums with grooves between them, fluted shaft, broken uneven top
# Vertex colour: R = per-piece tone, G = moss mask (up-facing), B = 1 on chipped/broken areas.
import json
import math
import os
import random

import bmesh
import bpy
from mathutils import Vector, noise

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
OUT = os.path.join(ROOT, "Generated", "Ruins")
DATA = json.load(open(os.path.join(OUT, "greybox.json")))


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def finish(bm, name, material, tone, seed, moss=True):
    bm.normal_update()
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    col = mesh.color_attributes.new("Col", "FLOAT_COLOR", "CORNER")
    for poly in mesh.polygons:
        poly.use_smooth = True
        up = poly.normal.z
        m = 0.0
        if moss:
            m = max(0.0, min(1.0, (up - 0.55) / 0.35))
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            var = 0.5 + 0.5 * noise.noise(co * 0.6 + Vector((seed, 0, 0)))
            col.data[li].color = (min(1.0, tone * 0.6 + var * 0.4), m, 0.0, 1.0)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(bpy.data.materials.get(material) or bpy.data.materials.new(material))
    # auto-smooth by angle so bevels read soft but faces stay flat
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    try:
        bpy.ops.object.shade_auto_smooth(angle=math.radians(40))
    except Exception:
        pass
    return obj


def block(dims, seed, wood=False):
    random.seed(seed)
    dx, dy, dz = dims
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((v.co.x * dx, v.co.y * dy, v.co.z * dz))
    small = min(dims)
    bevel = min(0.12 if not wood else 0.03, small * 0.18)
    bmesh.ops.bevel(bm, geom=list(bm.edges), offset=bevel, segments=2 if not wood else 1, affect="EDGES", profile=0.6)
    # subdivide big faces a little so the warp has vertices to move
    cuts = 2 if max(dims) > 3 else 1
    bmesh.ops.subdivide_edges(bm, edges=[e for e in bm.edges if e.calc_length() > 1.2], cuts=cuts, use_grid_fill=True)
    amp = 0.0 if wood else min(0.05, small * 0.04)
    for v in bm.verts:
        n = noise.noise(v.co * 0.35 + Vector((seed * 1.7, 0, 0)))
        v.co += v.normal * n * amp
    if not wood:
        # chip one or two top corners
        corners = sorted(bm.verts, key=lambda v: -(abs(v.co.x) / dx + abs(v.co.y) / dy + v.co.z / dz))
        for v in corners[:random.randint(2, 6)]:
            v.co -= v.normal * min(0.12, small * 0.12) * random.uniform(0.4, 1.0)
    return bm


def column(diam, height, seed, broken=True, wood=False):
    random.seed(seed)
    r = diam / 2
    bm = bmesh.new()
    seg = 24 if not wood else 12
    drums = max(1, round(height / (1.0 if not wood else height)))
    z0 = -height / 2
    rings = []
    for d in range(drums + 1):
        z = z0 + height * d / drums
        rings.append((z, 1.0))
        if 0 < d < drums:
            rings.append((z - 0.03, 0.94))       # groove between two drums
            rings.append((z + 0.03, 0.94))
    rings.sort()
    verts = []
    for z, s in rings:
        row = []
        for k in range(seg):
            a = k / seg * math.tau
            flute = 1.0 if wood else 1.0 - 0.035 * (0.5 + 0.5 * math.cos(a * 12))
            rr = r * s * flute
            row.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, z)))
        verts.append(row)
    for i in range(len(verts) - 1):
        for k in range(seg):
            bm.faces.new((verts[i][k], verts[i][(k + 1) % seg], verts[i + 1][(k + 1) % seg], verts[i + 1][k]))
    bot = bm.faces.new(list(reversed(verts[0])))
    top = bm.faces.new(verts[-1])
    if broken and not wood:
        # broken, uneven top: poke the cap and push its points down irregularly
        res = bmesh.ops.poke(bm, faces=[top])
        for v in res["verts"]:
            v.co.z -= random.uniform(0.05, 0.3)
        for v in verts[-1]:
            v.co.z -= random.uniform(0.0, 0.18) * (0.5 + noise.noise(Vector((v.co.x, v.co.y, seed))))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUT, obj.name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_yup=True,
                              export_apply=True, export_normals=True, export_vertex_color="ACTIVE")


for i, (key, piece) in enumerate(sorted(DATA["pieces"].items())):
    reset()
    wood = piece["style"] == "wood"
    tone = random.Random(key).random()
    if piece["shape"] == "Cylinder":
        bm = column(piece["dims"][0], piece["dims"][2], seed=i, broken=not wood, wood=wood)
    else:
        bm = block(piece["dims"], seed=i, wood=wood)
    obj = finish(bm, "SM_Ruin_" + key, "Wood" if wood else "Stone", tone, i, moss=not wood)
    export(obj)
    print("built", obj.name, len(obj.data.vertices))
