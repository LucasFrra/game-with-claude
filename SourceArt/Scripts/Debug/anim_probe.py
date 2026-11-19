# blender -b --factory-startup --python anim_probe.py -- <fbx> <out.png>
import sys, math, bpy
from mathutils import Vector
args = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=args[0], automatic_bone_orientation=False, primary_bone_axis="X", secondary_bone_axis="Y")
arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
print("armature", arm.name, "scale", tuple(arm.scale), "rot", tuple(arm.rotation_euler), "meshes", [m.name for m in meshes])
print("action", arm.animation_data.action.name if arm.animation_data and arm.animation_data.action else None, "frames", bpy.context.scene.frame_start, bpy.context.scene.frame_end)
names = [b.name for b in arm.data.bones]
print("bones", len(names))
for b in arm.data.bones:
    if b.name in ("root", "pelvis", "spine_01", "spine_03", "spine_05", "neck_01", "head", "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "thigh_l", "calf_l", "foot_l", "ball_l", "upperarm_r", "hand_r", "thigh_r", "foot_r"):
        print("  %-12s parent=%-12s head_world=%s" % (b.name, b.parent.name if b.parent else "-", tuple(round(c, 3) for c in (arm.matrix_world @ b.head_local))))
bpy.context.scene.frame_set(0)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); bpy.context.scene.collection.objects.link(cam)
cam.location = Vector((0, -4.0, 1.0)) * (1 if arm.matrix_world.to_scale().x > 0.5 else 1)
cam.rotation_euler = (math.radians(88), 0, 0)
bpy.context.scene.camera = cam
s = bpy.context.scene; s.render.engine = "BLENDER_WORKBENCH"; s.render.resolution_x = 512; s.render.resolution_y = 512; s.render.filepath = args[1]
bpy.ops.render.render(write_still=True)
