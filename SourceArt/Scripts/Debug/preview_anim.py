# blender -b --factory-startup --python preview_anim.py -- <out.png> <anim1.fbx>:<frame> [<anim2.fbx>:<frame> ...]
# Renders the mannequin mesh posed by each animation frame (side + front views), tiled.
import math, os, sys, bpy
from mathutils import Vector
args = sys.argv[sys.argv.index("--") + 1:]
out, items = args[0], args[1:]
root = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
tiles = []
for k, item in enumerate(items):
    path, frame = item.rsplit(":", 1)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(root, "Generated", "Anims", "Source", "MM_Idle.fbx"), automatic_bone_orientation=False, primary_bone_axis="X", secondary_bone_axis="Y")
    body = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
    for o in list(bpy.context.scene.objects):
        if o.type == "MESH" and not o.name.endswith("LOD0"):
            bpy.data.objects.remove(o, do_unlink=True)
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(root, "Generated", "Anims", path), automatic_bone_orientation=False, primary_bone_axis="X", secondary_bone_axis="Y")
    other = next(o for o in bpy.data.objects if o not in before and o.type == "ARMATURE")
    body.animation_data.action = other.animation_data.action
    bpy.data.objects.remove(other, do_unlink=True)
    bpy.context.scene.frame_set(int(frame))
    s = bpy.context.scene
    s.render.engine = "BLENDER_WORKBENCH"; s.render.resolution_x = 360; s.render.resolution_y = 480
    for view, loc, rot in (("side", (3.2, 0, 1.0), (math.radians(88), 0, math.radians(90))), ("front", (0, -3.2, 1.0), (math.radians(88), 0, 0))):
        cam = bpy.data.objects.new("cam_" + view, bpy.data.cameras.new("cam")); s.collection.objects.link(cam)
        cam.location = Vector(loc); cam.rotation_euler = rot; s.camera = cam
        f = out.replace(".png", "_%d_%s.png" % (k, view)); s.render.filepath = f
        bpy.ops.render.render(write_still=True)
        tiles.append(f)
from PIL import Image  # noqa: blender bundles no PIL -> handled outside
