# J4: imports the adventurer outfit (blender_outfit.py) and tints the mannequin like a tunic.
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Outfit"))
DST = "/Game/Project/Characters/Outfit"
MAT = "/Game/Project/Materials/Characters"
log = []

tasks = []
for f in sorted(os.listdir(SRC)):
    if f.endswith(".glb"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, f)
        t.destination_path = DST
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
L.TOOLS.import_asset_tasks(tasks)

two_tone = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_Distant")
mis = {
    "SM_Outfit_Hood": L.make_instance("MI_Outfit_Cloth", two_tone, MAT, None, None, {"Rock": (0.06, 0.17, 0.42), "Snow": (0.80, 0.72, 0.52)}),
    "SM_Outfit_Scarf": L.make_instance("MI_Outfit_Scarf", two_tone, MAT, None, None, {"Rock": (0.62, 0.16, 0.10), "Snow": (0.62, 0.16, 0.10)}),
    "SM_Outfit_Backpack": L.make_instance("MI_Outfit_Leather", two_tone, MAT, None, None, {"Rock": (0.22, 0.13, 0.07), "Snow": (0.55, 0.45, 0.30)}),
}
mis["SM_Outfit_Satchel"] = mis["SM_Outfit_Backpack"]
for p in L.EAL.list_assets(DST, recursive=True):
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh) and a.get_name() in mis:
        for i, _ in enumerate(a.get_editor_property("static_materials")):
            a.set_material(i, mis[a.get_name()])
        unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).remove_collisions(a)
        L.EAL.save_loaded_asset(a)
        log.append("%s -> %s" % (a.get_path_name(), mis[a.get_name()].get_name()))

# Mannequin tint: blue tunic, leather joints, no engine logo
MEL = unreal.MaterialEditingLibrary
body = L.make_instance("MI_Hero_Body", unreal.load_asset("/Game/Characters/Mannequins/Materials/Quinn/MI_Quinn_01"), MAT,
                       None, {"MetalPaintMetallic": 0.0, "MetalPaintRoughness": 0.85, "LogoScale": 0.0},
                       {"Paint Tint": (0.10, 0.24, 0.52), "LogoTint": (0.10, 0.24, 0.52)})
joints = L.make_instance("MI_Hero_Joints", unreal.load_asset("/Game/Characters/Mannequins/Materials/Quinn/MI_Quinn_02"), MAT,
                         None, {"MetalPaintMetallic": 0.0, "MetalPaintRoughness": 0.9, "LogoScale": 0.0},
                         {"Paint Tint": (0.20, 0.13, 0.08), "LogoTint": (0.20, 0.13, 0.08)})
bp = unreal.load_asset("/Game/Project/Characters/BP_EldoriaCharacter")
cdo = unreal.get_default_object(bp.generated_class())
cdo.get_editor_property("mesh").set_editor_property("override_materials", [body, joints])
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
L.EAL.save_loaded_asset(bp)
log.append("hero tint applied")
with open(os.path.join(unreal.Paths.project_saved_dir(), "Outfit.txt"), "w") as f:
    f.write("\n".join(log))
