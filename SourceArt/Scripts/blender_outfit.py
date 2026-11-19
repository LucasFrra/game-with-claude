# Project Eldoria - J4 adventurer outfit (rigid pieces attached to mannequin bones), headless:
#   blender -b --factory-startup --python blender_outfit.py
# Modelled in "actor space" around the attach bone: X forward, Y right, Z up, metres, origin at the bone.
# Vertex colour R picks the material tone (0 main colour, 1 trim) for M_Eldoria_Distant-style two-tone MIs.
import math
import os

import bmesh
import bpy
from mathutils import Vector, noise

OUT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Outfit"))
os.makedirs(OUT, exist_ok=True)


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def build(name, bm, material, trim_fn=None, smooth=True):
    bm.normal_update()
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    col = mesh.color_attributes.new("Col", "FLOAT_COLOR", "CORNER")
    for poly in mesh.polygons:
        poly.use_smooth = smooth
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            t = trim_fn(co) if trim_fn else 0.0
            col.data[li].color = (t, 0.0, 0.0, 1.0)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(bpy.data.materials.new(material))
    return obj


def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    mod = obj.modifiers.new("solid", "SOLIDIFY") if obj.get("solidify") else None
    if mod:
        mod.thickness = obj["solidify"]
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, obj.name + ".glb"), use_selection=True, export_format="GLB",
                              export_yup=True, export_apply=True, export_vertex_color="ACTIVE")
    print("exported", obj.name)


def hood():
    """Hood + short shoulder cape. Origin = head bone (base of the skull)."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=40, v_segments=24, radius=1.0)
    for v in bm.verts:
        x, y, z = v.co
        # skull dome, a point at the back of the crown
        v.co = Vector((x * 0.135 - 0.01, y * 0.125, z * 0.15 + 0.115))
        if z > 0.3 and x < 0:
            v.co.x -= 0.045 * (z - 0.3) * (-x)
            v.co.z += 0.03 * (z - 0.3) * (-x)
        # flare the lower part into a capelet over the shoulders
        if z < -0.2:
            k = (-0.2 - z) / 0.8
            v.co.x *= 1.0 + 0.9 * k
            v.co.y *= 1.0 + 1.9 * k
            v.co.z -= 0.07 * k
        v.co += v.co.normalized() * 0.006 * noise.noise(v.co * 25)
    # face opening
    gone = [f for f in bm.faces if f.calc_center_median().x > 0.06 and -0.06 < f.calc_center_median().z < 0.2 and abs(f.calc_center_median().y) < 0.09]
    bmesh.ops.delete(bm, geom=gone, context="FACES")
    obj = build("SM_Outfit_Hood", bm, "Cloth", trim_fn=lambda co: 1.0 if co.z < -0.02 else 0.0)
    obj["solidify"] = 0.012
    return obj


def scarf():
    """Scarf around the neck with a tail falling on the back. Origin = neck_01."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=False, segments=28, radius1=0.085, radius2=0.075, depth=0.07)
    for v in bm.verts:
        v.co.z += 0.03 + 0.012 * math.sin(math.atan2(v.co.y, v.co.x) * 3)
        v.co.x *= 1.05
    tail = bmesh.ops.create_grid(bm, x_segments=2, y_segments=6, size=0.5)
    for v in tail["verts"]:
        y0, z0 = v.co.x, v.co.y
        v.co = Vector((-0.085 - 0.04 * (z0 + 0.5), 0.03 + y0 * 0.1, 0.0 - (z0 + 0.5) * 0.3))
        v.co.x -= 0.03 * math.sin((z0 + 0.5) * 3)
    obj = build("SM_Outfit_Scarf", bm, "Scarf")
    obj["solidify"] = 0.01
    return obj


def rounded_box(bm, size, center, bevel):
    res = bmesh.ops.create_cube(bm, size=1.0)
    verts = res["verts"]
    for v in verts:
        v.co = Vector((v.co.x * size[0], v.co.y * size[1], v.co.z * size[2])) + Vector(center)
    edges = list({e for v in verts for e in v.link_edges})
    bmesh.ops.bevel(bm, geom=edges, offset=bevel, segments=3, affect="EDGES")


def backpack():
    """Pack + bedroll. Origin = spine_03, pack sits behind the back."""
    bm = bmesh.new()
    rounded_box(bm, (0.15, 0.30, 0.34), (-0.16, 0.0, -0.02), 0.035)
    rounded_box(bm, (0.035, 0.24, 0.12), (-0.25, 0.0, -0.06), 0.012)       # front pocket
    roll = bmesh.ops.create_cone(bm, cap_ends=True, segments=20, radius1=0.07, radius2=0.07, depth=0.38)
    for v in roll["verts"]:
        v.co = Vector((v.co.x - 0.16, v.co.z, v.co.y + 0.2))
    for s in (-1, 1):                                                          # straps over the shoulders
        rounded_box(bm, (0.12, 0.035, 0.02), (-0.04, s * 0.09, 0.15), 0.008)
    return build("SM_Outfit_Backpack", bm, "Leather", trim_fn=lambda co: 1.0 if co.z > 0.13 else 0.0)


def satchel():
    """Small bag on the hip. Origin = pelvis."""
    bm = bmesh.new()
    rounded_box(bm, (0.13, 0.07, 0.15), (0.02, 0.2, -0.08), 0.02)
    rounded_box(bm, (0.135, 0.075, 0.05), (0.02, 0.2, -0.02), 0.012)       # flap
    rounded_box(bm, (0.30, 0.42, 0.035), (0.0, 0.0, 0.01), 0.012)          # belt (ring around the waist)
    obj = build("SM_Outfit_Satchel", bm, "Leather", trim_fn=lambda co: 1.0 if co.z > -0.01 and abs(co.y) < 0.2 else 0.0)
    return obj


for fn in (hood, scarf, backpack, satchel):
    reset()
    export(fn())
