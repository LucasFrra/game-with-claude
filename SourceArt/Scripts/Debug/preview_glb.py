# blender -b --factory-startup --python preview_glb.py -- <glb> <out.png> <distance>
import sys, math, bpy
from mathutils import Vector
args = sys.argv[sys.argv.index("--") + 1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=args[0])
objs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
mins = Vector((min(v[0] for o in objs for v in o.bound_box), min(v[1] for o in objs for v in o.bound_box), min(v[2] for o in objs for v in o.bound_box)))
maxs = Vector((max(v[0] for o in objs for v in o.bound_box), max(v[1] for o in objs for v in o.bound_box), max(v[2] for o in objs for v in o.bound_box)))
c = (mins + maxs) / 2; size = (maxs - mins).length
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam")); bpy.context.scene.collection.objects.link(cam)
cam.location = c + Vector((size * 0.9, -size * 0.9, size * 0.35)); cam.rotation_euler = (c - cam.location).to_track_quat("-Z", "Y").to_euler()
bpy.context.scene.camera = cam
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN")); sun.rotation_euler = (0.8, 0.2, 0.6); bpy.context.scene.collection.objects.link(sun)
s = bpy.context.scene; s.render.engine = "BLENDER_WORKBENCH"; s.display.shading.light = "STUDIO"; s.display.shading.color_type = "VERTEX"
s.render.resolution_x = s.render.resolution_y = 512; s.render.filepath = args[1]
bpy.ops.render.render(write_still=True)
print("dims", maxs - mins)
