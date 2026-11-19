# Project Eldoria - imports the generated jungle terrain and places it in Lvl_TharumJungle.
import os
import unreal

SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Terrain", "SM_Terrain_TharumJungle.gltf"))
DEST = "/Game/Project/Environment/Terrain"
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "TerrainReport.txt")

lines = []

task = unreal.AssetImportTask()
task.filename = SRC
task.destination_path = DEST
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
lines.append("imported: " + ", ".join(task.imported_object_paths))

mesh = None
for p in task.imported_object_paths:
    asset = unreal.load_asset(p)
    if isinstance(asset, unreal.StaticMesh):
        mesh = asset
if mesh is None:
    for p in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
        asset = unreal.load_asset(p)
        if isinstance(asset, unreal.StaticMesh):
            mesh = asset

if mesh:
    body = mesh.get_editor_property("body_setup")
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    bounds = mesh.get_bounds()
    lines.append("mesh %s origin %s extent %s" % (mesh.get_path_name(), bounds.origin, bounds.box_extent))

    level = "/Game/Project/Levels/Lvl_TharumJungle"
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if les.get_current_level() is None or "Lvl_TharumJungle" not in str(unreal.EditorLevelLibrary.get_editor_world().get_path_name()):
        les.load_level(level)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == "Terrain_TharumJungle":
            eas.destroy_actor(a)
    actor = eas.spawn_actor_from_object(mesh, unreal.Vector(0, 0, 0))
    actor.set_actor_label("Terrain_TharumJungle")
    actor.set_folder_path("Jungle/Terrain")
    lines.append("actor bounds origin/extent: %s" % (actor.get_actor_bounds(False),))
    les.save_current_level()
else:
    lines.append("NO MESH FOUND")

with open(REPORT, "w") as f:
    f.write("\n".join(str(l) for l in lines))
unreal.log("Eldoria terrain: " + " | ".join(str(l) for l in lines))
