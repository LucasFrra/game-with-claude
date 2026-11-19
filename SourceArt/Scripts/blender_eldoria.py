# Project Eldoria - Blender modelling helpers (run inside Blender via exec).
# Every part is modelled at its final size in metres, centred on the origin, so that in Unreal it
# replaces a basic shape component 1:1 (or, for "unit" parts, keeps the component's scale).
import math
import os
import random

import bmesh
import bpy
from mathutils import Vector, noise

PROJECT_ROOT = os.environ.get("ELDORIA_ROOT", os.path.join(os.path.expanduser("~"), "Documents", "Unreal Projects", "ProjectEldoria"))
OUT_DIR = os.path.join(PROJECT_ROOT, "SourceArt", "Generated", "Tharum")
SCENE_NAME = "EldoriaAssets"


def use_scene():
    scene = bpy.data.scenes.get(SCENE_NAME) or bpy.data.scenes.new(SCENE_NAME)
    bpy.context.window.scene = scene
    return scene


def clear_object(name):
    obj = bpy.data.objects.get(name)
    if obj:
        mesh = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        if mesh and mesh.users == 0:
            bpy.data.meshes.remove(mesh)


def material(name, rgb):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (rgb[0], rgb[1], rgb[2], 1.0)
    return mat


def new_object(name, bm, materials=()):
    clear_object(name)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for mat in materials:
        mesh.materials.append(mat)
    for poly in mesh.polygons:
        poly.use_smooth = True
    return obj


def uv_sphere(segments=48, rings=24, radius=0.5):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius)
    return bm


def cylinder(segments=32, radius=0.5, depth=1.0, cap=True):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=cap, cap_tris=False, segments=segments, radius1=radius, radius2=radius, depth=depth)
    return bm


def deform(bm, fn):
    for v in bm.verts:
        v.co = fn(v.co.copy())


def wrinkle(bm, amount, scale, seed=0.0):
    """Organic surface noise along the normals (wrinkled skin, chipped stone)."""
    bm.normal_update()
    for v in bm.verts:
        n = noise.noise((v.co * scale) + Vector((seed, seed * 0.7, seed * 1.3)))
        v.co += v.normal * n * amount


def finish(obj, subdivide=1, smooth_angle=None):
    if subdivide:
        mod = obj.modifiers.new("Subsurf", "SUBSURF")
        mod.levels = subdivide
        mod.render_levels = subdivide
    bpy.context.view_layer.objects.active = obj
    for o in bpy.context.selected_objects:
        o.select_set(False)
    obj.select_set(True)
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    if smooth_angle is not None:
        try:
            bpy.ops.object.shade_auto_smooth(angle=math.radians(smooth_angle))
        except Exception:
            bpy.ops.object.shade_smooth()
    return obj


def export(obj):
    os.makedirs(OUT_DIR, exist_ok=True)
    for o in bpy.context.selected_objects:
        o.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUT_DIR, obj.name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_yup=True, export_apply=True)
    return path


def dims(obj):
    return tuple(round(d, 2) for d in obj.dimensions)


def smoothstep(e0, e1, x):
    t = max(0.0, min(1.0, (x - e0) / (e1 - e0)))
    return t * t * (3 - 2 * t)


def gauss(x, mu, sigma):
    return math.exp(-((x - mu) ** 2) / (2 * sigma * sigma))
