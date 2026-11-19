import sys, os, bpy
args = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=args[0], automatic_bone_orientation=False, primary_bone_axis="X", secondary_bone_axis="Y")
for o in bpy.data.objects:
    print("obj", o.name, o.type, tuple(round(v,3) for v in o.scale))
for a in bpy.data.actions:
    fc = list(a.fcurves) if hasattr(a, "fcurves") else []
    print("action", a.name, "range", tuple(a.frame_range), "fcurves", len(fc), [f.data_path for f in fc[:3]])
    try:
        print("  layers", len(a.layers), "slots", [s.identifier for s in a.slots])
    except Exception as e:
        print("  no layered api", e)
arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
for f in (1, 10):
    bpy.context.scene.frame_set(f)
    pb = arm.pose.bones["upperarm_l"]
    print("frame", f, "upperarm_l q", tuple(round(v, 3) for v in pb.rotation_quaternion), "hand_l head", tuple(round(v, 1) for v in arm.pose.bones["hand_l"].head))
