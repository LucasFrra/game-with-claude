# J4: imports the Blender-authored animations (blender_anims.py) onto the UE mannequin skeleton.
import os
import unreal

SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Anims"))
DST = "/Game/Project/Characters/Anims"
log = []
skeleton = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
log.append("skeleton %s" % skeleton)
tasks = []
for f in sorted(os.listdir(SRC)):
    if f.endswith(".fbx") and f.startswith("A_"):
        ui = unreal.FbxImportUI()
        ui.set_editor_property("import_mesh", False)
        ui.set_editor_property("import_animations", True)
        ui.set_editor_property("import_materials", False)
        ui.set_editor_property("import_textures", False)
        ui.set_editor_property("skeleton", skeleton)
        ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        ad = ui.get_editor_property("anim_sequence_import_data")
        ad.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        ad.set_editor_property("remove_redundant_keys", False)
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, f)
        t.destination_path = DST
        t.destination_name = f[:-4]
        t.automated = True
        t.replace_existing = True
        t.save = True
        t.options = ui
        tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    log.append("%s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)))
for p in unreal.EditorAssetLibrary.list_assets(DST, recursive=True):
    a = unreal.load_asset(p)
    if isinstance(a, unreal.AnimSequence):
        log.append("  %s length %.2fs skeleton %s" % (a.get_name(), a.get_play_length(), a.get_editor_property("skeleton").get_name()))
with open(os.path.join(unreal.Paths.project_saved_dir(), "ImportAnims.txt"), "w") as f:
    f.write("\n".join(str(x) for x in log))
