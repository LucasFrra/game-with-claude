# J3: lists every greybox piece (engine Cube / Cylinder components) with its exact size, so Blender can build a
# stylized replacement of the same dimensions (blender_ruins.py). Output: SourceArt/Generated/Ruins/greybox.json
import json
import os
import unreal

OUT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Ruins"))
os.makedirs(OUT, exist_ok=True)
SKIP_CLASSES = (unreal.EldoriaTharum, unreal.EldoriaControlDevice)
pieces = {}
items = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(a, SKIP_CLASSES):
        continue
    label = a.get_actor_label()
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        if isinstance(c, unreal.InstancedStaticMeshComponent) or not c.static_mesh:
            continue
        shape = c.static_mesh.get_name()
        if shape not in ("Cube", "Cylinder"):
            continue
        if not c.is_visible() or label.startswith(("TrunkCollider_", "Pool_Water")):
            continue
        s = c.get_world_scale()
        dims = [round(abs(s.x), 2), round(abs(s.y), 2), round(abs(s.z), 2)]      # metres (basic shapes are 1 m)
        if shape == "Cylinder":
            d = round((dims[0] + dims[1]) / 2, 2)
            dims = [d, d, dims[2]]
        key = "%s_%s" % ("Col" if shape == "Cylinder" else "Blk", "x".join("%03d" % int(round(v * 100)) for v in dims))
        style = "wood" if label.startswith(("Collapsible_Hut", "Collapsible_Scaffold", "Log_", "Crate_")) else "stone"
        key = key + ("_W" if style == "wood" else "")
        pieces[key] = {"shape": shape, "dims": dims, "style": style}
        items.append({"actor": label, "component": c.get_name(), "key": key})
with open(os.path.join(OUT, "greybox.json"), "w") as f:
    json.dump({"pieces": pieces, "items": items}, f, indent=1)
with open(os.path.join(unreal.Paths.project_saved_dir(), "Greybox.txt"), "w") as f:
    f.write("%d pieces, %d items\n" % (len(pieces), len(items)))
    f.write("\n".join(sorted(set(i["actor"].rstrip("0123456789") for i in items))))
